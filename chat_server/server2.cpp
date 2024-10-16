#include <iostream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <arpa/inet.h>          // inet_ntop()
#include "message.pb.h"         // Protobuf 메시지 정의

#define MAX_BUFFER_SIZE 100

using namespace std;
using namespace mju;

class Client {
public:
    Client() : socket_(-1) {}
    Client(int socket) : socket_(socket) {}

    int GetSocket() const { return socket_; }

private:
    int socket_;
};

class Server {
public:
    Server(int port, int num_thread) : num_thread_(num_thread) {
        BindConnection(port);
        threads.resize(num_thread_);
    }

    ~Server() {
        for (int i = 0; i < num_thread_; i++) {
            threads[i].join();
        }
    }

    void Run(Type* t) {
        for (int i = 0; i < num_thread_; i++) {
            cout << "메시지 작업 쓰레드 #" << i << " 생성" << endl;
            threads.emplace_back(&Server::WorkerThread, this, t);
        }

        while (1) {
            temp_rfds = rfds;

            int active_socket_count = select(max_fd + 1, &temp_rfds, NULL, NULL, NULL);

            if (active_socket_count <= 0) {
                cerr << "select() failed: " << strerror(errno) << endl;
                break;
            }

            if (FD_ISSET(passive_sock, &temp_rfds)) {
                AcceptConnection();
            }

            for (int i = 0; i < active_socket_count; i++) {
                Client &client = clients[i];

                if (client.GetSocket() == passive_sock) {
                    continue;
                }

                if (FD_ISSET(client.GetSocket(), &temp_rfds)) {
                    ReceiveData(client);
                }
            }
        }
    }

private:
    void BindConnection(int port) {
        passive_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (passive_sock < 0) {
            cerr << "socket() failed: " << strerror(errno) << endl;
            exit(1);
        }

        struct sockaddr_in server_sin;
        memset(&server_sin, 0, sizeof(server_sin));
        server_sin.sin_family = AF_INET;
        server_sin.sin_addr.s_addr = INADDR_ANY;
        server_sin.sin_port = htons(port);

        int on = 1;
        setsockopt(passive_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

        if (bind(passive_sock, (struct sockaddr *)&server_sin, sizeof(server_sin)) < 0) {
            cerr << "bind() failed: " << strerror(errno) << endl;
            exit(1);
        }

        cout << "Port 번호 " << port << "에서 서버 동작 중" << endl;

        if (listen(passive_sock, 10) < 0) {
            cerr << "listen() failed: " << strerror(errno) << endl;
            exit(1);
        }

        FD_ZERO(&rfds);
        FD_SET(passive_sock, &rfds);
        max_fd = passive_sock;
    }

    void AcceptConnection() {
        struct sockaddr_in client_sin;
        memset(&client_sin, 0, sizeof(client_sin));
        unsigned int client_sin_len = sizeof(client_sin);
        int client_sock = accept(passive_sock, (struct sockaddr *)&client_sin, &client_sin_len);
        if (client_sock < 0) {
            cerr << "accept() failed: " << strerror(errno) << endl;
            exit(1);
        }

        inet_ntop(AF_INET, &(client_sin.sin_addr), client_ip, INET_ADDRSTRLEN);
        client_port = ntohs(client_sin.sin_port);

        SetNonBlocking(client_sock);
        FD_SET(client_sock, &rfds);
        max_fd = max(max_fd, client_sock);

        clients.emplace_back(client_sock);
        cout << "새로운 클라이언트 접속 [('" << client_ip << "', " << client_port << ")]" << endl;
    }

    void SetNonBlocking(int socket) {
        int flag = fcntl(socket, F_GETFL, 0);
        if (flag < 0) {
            cerr << "fcntl() failed: " << strerror(errno) << endl;
            exit(1);
        }

        if (fcntl(socket, F_SETFL, flag | O_NONBLOCK) < 0) {
            cerr << "fcntl() failed: " << strerror(errno) << endl;
            exit(1);
        }
    }

    void ReceiveData(Client &client) {
        static char buffer[MAX_BUFFER_SIZE];
        static size_t total_bytes = 0;

        ssize_t recv_bytes = recv(client.GetSocket(), buffer + total_bytes, sizeof(buffer) - total_bytes, 0);
        int socket = client.GetSocket();

        if (recv_bytes <= 0) {      // 에러 처리
            if (recv_bytes == 0) {
                cout << "클라이언트 [('" << client_ip << "', " << client_port << "):" << " ]: 상대방이 소켓을 닫았음" << endl;
            } else {
                cerr << "recv() failed: " << strerror(errno) << endl;
            }

            close(socket);
            FD_CLR(socket, &rfds);

            {
                unique_lock<mutex> lock(m);
                clients.erase(remove_if(clients.begin(), clients.end(), [socket](const Client &c) { return c.GetSocket() == socket; }), clients.end());
            }
        }

        total_bytes += recv_bytes;
        //cout << "Received data (bytes): " << total_bytes << endl;

        while (total_bytes >= 2) {      // 최소 메시지 크기인 2bytes 이상은 있어야 함
            uint16_t message_size;
            memcpy(&message_size, buffer, sizeof(message_size));
            message_size = ntohs(message_size);     // 클라이언트 측에서 network byte order로 전송하기 때문에 서버 측에서 host byte order로 변환해야 함

            // message_size에 따라 충분한 데이터가 있는지 확인
            if (total_bytes < message_size + 2) {
                cout << "[Error] 메시지가 모두 도착하지 않았습니다." << endl;
                return;
            }

            // 타입 메시지 파싱
            Type type_message;
            if (!type_message.ParseFromArray(buffer + 2, message_size)) {
                cerr << "[Error] Type 메시지 파싱에 실패했습니다." << endl;
                return;
            }

            // 타입 메시지 출력
            //cout << "Type message(enum) received: " << type_message.type() << endl;

            // 처리할 메시지 크기
            size_t real_data_bytes = 2 + message_size;

            switch (type_message.type()) {
                case Type::CS_NAME:{
                    // 다음 메시지 크기 읽기 (채팅 텍스트)
                    if (total_bytes < real_data_bytes) {
                        cout << "[Error] 메시지가 모두 도착하지 않았습니다." << endl;
                        return;
                    }

                    // 다음 2바이트를 읽어 메시지 크기
                    uint16_t chat_message_size;
                    memcpy(&chat_message_size, buffer + real_data_bytes, sizeof(chat_message_size));
                    chat_message_size = ntohs(chat_message_size);

                    // 메시지 크기에 따라 데이터가 있는지 확인
                    if (total_bytes < real_data_bytes + 2 + chat_message_size) {
                        //cout << "[Error] 메시지가 모두 도착하지 않았습니다." << endl;
                        return;
                    }

                    // 채팅 메시지 파싱
                    CSName name_message;
                    if (!name_message.ParseFromArray(buffer + real_data_bytes + 2, chat_message_size)) {
                        cerr << "Failed to parse CSName message." << endl;
                    } else {
                        // 메시지 처리 (예: 채팅 메시지 출력)
                        cout << "[" << client_port << "] 유저의 이름이 " << name_message.name() << " (으)로 변경되었습니다." << endl;
                    }

                    real_data_bytes += 2 + chat_message_size; // 채팅 메시지 크기 추가
                    break;
                }
                case Type::CS_CREATE_ROOM: {
                    // 다음 메시지 크기 읽기 (채팅 텍스트)
                    if (total_bytes < real_data_bytes) {
                        cout << "[Error] 메시지가 모두 도착하지 않았습니다." << endl;
                        return;
                    }

                    // 다음 2바이트를 읽어 메시지 크기
                    uint16_t chat_message_size;
                    memcpy(&chat_message_size, buffer + real_data_bytes, sizeof(chat_message_size));
                    chat_message_size = ntohs(chat_message_size);

                    // 메시지 크기에 따라 데이터가 있는지 확인
                    if (total_bytes < real_data_bytes + 2 + chat_message_size) {
                        //cout << "[Error] 메시지가 모두 도착하지 않았습니다." << endl;
                        return;
                    }

                    // 채팅 메시지 파싱
                    CSCreateRoom room_message;
                    if (!room_message.ParseFromArray(buffer + real_data_bytes + 2, chat_message_size)) {
                        cerr << "Failed to parse CSCreateRoom message." << endl;
                    } else {
                        // 메시지 처리 (예: 채팅 메시지 출력)
                        cout << "[" << client_port << "] 유저가 \"" << room_message.title() << "\" 방을 생성했습니다. "  << endl;
                    }

                    real_data_bytes += 2 + chat_message_size; // 채팅 메시지 크기 추가
                    break;
                }
                case Type::CS_JOIN_ROOM:
                    break;
                case Type::CS_CHAT: {
                    // 다음 메시지 크기 읽기 (채팅 텍스트)
                    if (total_bytes < real_data_bytes) {
                        cout << "[Error] 메시지가 모두 도착하지 않았습니다." << endl;
                        return;
                    }

                    // 다음 2바이트를 읽어 메시지 크기
                    uint16_t chat_message_size;
                    memcpy(&chat_message_size, buffer + real_data_bytes, sizeof(chat_message_size));
                    chat_message_size = ntohs(chat_message_size);

                    // 메시지 크기에 따라 데이터가 있는지 확인
                    if (total_bytes < real_data_bytes + 2 + chat_message_size) {
                        //cout << "[Error] 메시지가 모두 도착하지 않았습니다." << endl;
                        return;
                    }

                    // 채팅 메시지 파싱
                    CSChat chat_message;
                    if (!chat_message.ParseFromArray(buffer + real_data_bytes + 2, chat_message_size)) {
                        cerr << "Failed to parse CSChat message." << endl;
                    } else {
                        // 메시지 처리 (예: 채팅 메시지 출력)
                        cout << "[" << client_port << "]: " << chat_message.text() << endl;
                    }

                    real_data_bytes += 2 + chat_message_size; // 채팅 메시지 크기 추가
                    break;
                }
                case Type::CS_ROOMS:
                    break;
                case Type::CS_LEAVE_ROOM:
                    break;
                case Type::CS_SHUTDOWN:
                    break;
                default:
                    cerr << "Unknown message type." << endl;
                    return;
            }

            // 처리된 데이터 크기만큼 buffer에서 제거
            memmove(buffer, buffer + real_data_bytes, total_bytes - real_data_bytes);
            total_bytes -= real_data_bytes;
        }
    }

    bool RecvErrorHandling(int socket, ssize_t recv_bytes) {
        if (recv_bytes <= 0) {
            if (recv_bytes == 0) {
                cout << "클라이언트 [('" << client_ip << "', " << client_port << "):" << " ]: 상대방이 소켓을 닫았음" << endl;
            } else {
                cerr << "recv() header failed: " << strerror(errno) << endl;
            }

            close(socket);
            FD_CLR(socket, &rfds);
            clients.erase(remove_if(clients.begin(), clients.end(), [socket](const Client &c) { return c.GetSocket() == socket; }), clients.end());
            return true;
        } else {
            return false;
        }
    }

    void WorkerThread(Type *t) {
        while (true) {
            Client c;
            {
                unique_lock<mutex> lock(m);
                cv.wait(lock, [this] { return !clients.empty(); });

                if (!clients.empty()) {
                    c = move(clients.front());
                    clients.erase(clients.begin());
                }
            }

            if (c.GetSocket() < 0) {
                continue;
            }
            // ReceiveAndBroadcastChatMessages(c);
        }
    }

private:
    int passive_sock;
    fd_set rfds, temp_rfds;
    int max_fd;
    vector<Client> clients;
    char client_ip[20];
    int client_port;

    int num_thread_;
    vector<thread> threads;

    mutex m;
    condition_variable cv;

    int next_room_id = 1;
};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        return 1;
    }

    int port = stoi(argv[1]);
    int num_thread = stoi(argv[2]);

    Server server(port, num_thread);
    Type *t = new Type;
    server.Run(t);

    delete t;
    return 0;
}