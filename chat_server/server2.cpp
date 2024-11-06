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
    Client() : socket_(-1), port_(0) {}
    Client(int socket, const string& ip, int port) : socket_(socket), ip_(ip), port_(port) {}

    int GetSocket() const { return socket_; }
    string GetIp() const { return ip_; }
    int GetPort() const { return port_; }

    string GetName() const { return name_; }
    int GetRoomId() const { return room_id_; }

    void SetName(string name) { name_ = name; }
    void SetRoomId(int room_id) { room_id_ = room_id; }

private:
    int socket_;
    string ip_;
    int port_;

    string name_ = "unknown";
    int room_id_ = -1;
};

class Room {
public:
    Room(): id_(-1), title_(""), member_num_(0){}       // 기본 생성자 추가
    Room(int id, const string& title) : id_(id), title_(title) {}

    int GetMemberNum() const { return member_num_; }
    void JoinMember() { member_num_++; }
    void LeaveMember() { member_num_--; }

    int id_;
    string title_;           // 방 제목
    int member_num_;       // 방 멤버 수
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

                for (size_t i = 0; i < clients.size(); i++) {       // 모든 소켓에 대한 이벤트를 체크하도록 수정
                    if (FD_ISSET(clients[i].GetSocket(), &temp_rfds)) {
                        ReceiveData(clients[i]);
                    }
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

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_sin.sin_addr), client_ip, INET_ADDRSTRLEN);
        client_port = ntohs(client_sin.sin_port);

        SetNonBlocking(client_sock);
        FD_SET(client_sock, &rfds);
        max_fd = max(max_fd, client_sock);

        clients.emplace_back(client_sock, client_ip, client_port);
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
                cout << "클라이언트 [('" << client.GetIp() << "', " << client.GetPort() << "):" << " ]: 상대방이 소켓을 닫았음" << endl;
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

        while (total_bytes >= 2) {      // 최소 메시지 크기인 2bytes 이상은 있어야 함
            uint16_t message_size;
            memcpy(&message_size, buffer, sizeof(message_size));
            message_size = ntohs(message_size);     // 클라이언트 측에서 network byte order로 전송하기 때문에 서버 측에서 host byte order로 변환해야 함

            // message_size에 따라 충분한 데이터가 있는지 확인
            if (total_bytes < 2 + message_size) {
                return;
            }

            // 타입 메시지 파싱
            Type type_message;
            if (!type_message.ParseFromArray(buffer + 2, message_size)) {
                //cerr << "[Error] Type 메시지 파싱에 실패했습니다. 전체 메시지를 건너뜁니다." << endl;
                
                // 메시지 크기만큼 전체를 건너뛰고 다음 메시지로 이동 (한 번 파싱 실패했을 때 다 밀리는 문제 방지)
                memmove(buffer, buffer + 2 + message_size, total_bytes - (2 + message_size));
                total_bytes -= (2 + message_size);

                continue;
            }

            // 타입 메시지 출력
            //cout << "Type message(enum) received: " << type_message.type() << endl;

            // 처리할 메시지 크기
            size_t real_data_bytes = 2 + message_size;

            switch (type_message.type()) {
                case Type::CS_CREATE_ROOM: {
                    // 다음 메시지 크기 읽기 (채팅 텍스트)
                    if (total_bytes < real_data_bytes) {
                        return;
                    }

                    // 다음 2바이트를 읽어 메시지 크기
                    uint16_t room_message_size;
                    memcpy(&room_message_size, buffer + real_data_bytes, sizeof(room_message_size));
                    room_message_size = ntohs(room_message_size);

                    // 메시지 크기에 따라 데이터가 있는지 확인
                    if (total_bytes < real_data_bytes + 2 + room_message_size) {
                        return;
                    }

                    CSCreateRoom room_message;
                    if (!room_message.ParseFromArray(buffer + real_data_bytes + 2, room_message_size)) {
                        cerr << "Failed to parse CSCreateRoom message." << endl;
                    } else {
                        string room_title = room_message.title();
                        int room_id = next_room_id++;

                        Room new_room(room_id, room_title);
                        rooms[room_id] = new_room;      // 방 목록에 새 방 추가

                        client.SetRoomId(room_id);      // 방 만들면서 방장이 방에 들어감
                        new_room.JoinMember();          // 방 멤버 수 추가

                        cout << "[시스템 메시지] " << client.GetName() << " 유저가 \"" << room_title << "\" 방을 생성했습니다. "  << endl;
                    }

                    real_data_bytes += 2 + room_message_size;
                    break;
                }
                case Type::CS_JOIN_ROOM: {
                    if (total_bytes < real_data_bytes) {
                        return;
                    }
                    
                    uint16_t room_id_size;
                    memcpy(&room_id_size, buffer + real_data_bytes, sizeof(room_id_size));
                    room_id_size = ntohs(room_id_size);

                    if (total_bytes < real_data_bytes + 2 + room_id_size) {
                        return;
                    }

                    // 방 ID 파싱
                    CSJoinRoom join_room_message;
                    if (!join_room_message.ParseFromArray(buffer + real_data_bytes + 2, room_id_size)) {
                        cerr << "Failed to parse CSJoinRoom message." << endl;
                    } else {
                        int room_id = join_room_message.roomid();

                        // 방이 존재하는지 확인
                        if(rooms[room_id].id_ != -1){
                            // 방에 입장
                            client.SetRoomId(room_id);
                            rooms[room_id].JoinMember();
                            cout << "[시스템 메시지] " << client.GetName() << " 유저가 방 " << room_id << "에 입장했습니다." << endl;
                        } else {
                            // 방이 존재하지 않는 경우 처리
                            cerr << "존재하지 않는 방에 들어가려고 했습니다: " << room_id << endl;
                        }
                    }

                    real_data_bytes += 2 + room_id_size;
                    break;
                }
                case Type::CS_CHAT: {
                    // 다음 메시지 크기 읽기 (채팅 텍스트)
                    if (total_bytes < real_data_bytes) {
                        return;
                    }

                    // 다음 2바이트를 읽어 메시지 크기
                    uint16_t chat_message_size;
                    memcpy(&chat_message_size, buffer + real_data_bytes, sizeof(chat_message_size));
                    chat_message_size = ntohs(chat_message_size);

                    // 메시지 크기에 따라 데이터가 있는지 확인
                    if (total_bytes < real_data_bytes + 2 + chat_message_size) {
                        return;
                    }

                    // 채팅 메시지 파싱
                    CSChat chat_message;
                    if (!chat_message.ParseFromArray(buffer + real_data_bytes + 2, chat_message_size)) {
                        cerr << "Failed to parse CSChat message." << endl;
                    } else {
                        // 메시지 처리 (예: 채팅 메시지 출력)
                        int room_id = client.GetRoomId();

                        if(room_id != -1){
                            string title = rooms[client.GetRoomId()].title_;
                            cout << "[" << title << "] " << "[" << client.GetName() << "]: " << chat_message.text() << endl;
                        } else{
                            cout << "[로비] [" << client.GetName() << "]: " << chat_message.text() << endl;
                        }
                    }

                    real_data_bytes += 2 + chat_message_size; // 채팅 메시지 크기 추가
                    break;
                }
                case Type::CS_NAME: {
                    // 다음 메시지 크기 읽기 (채팅 텍스트)
                    if (total_bytes < real_data_bytes) {
                        return;
                    }

                    // 다음 2바이트를 읽어 메시지 크기
                    uint16_t name_message_size;
                    memcpy(&name_message_size, buffer + real_data_bytes, sizeof(name_message_size));
                    name_message_size = ntohs(name_message_size);

                    // 메시지 크기에 따라 데이터가 있는지 확인
                    if (total_bytes < real_data_bytes + 2 + name_message_size) {
                        return;
                    }

                    ProcessNameCommand(client, buffer, real_data_bytes, name_message_size);

                    real_data_bytes += 2 + name_message_size; // 채팅 메시지 크기 추가
                    break;
                }
                case Type::CS_ROOMS:
                    break;
                case Type::CS_LEAVE_ROOM: {
                    int room_id = client.GetRoomId();

                    // 해당 클라가 속해있는 방을 찾는다.
                    auto it = find_if(rooms.begin(), rooms.end(), [room_id](const Room &room) {
                        return room.id_ == room_id;
                    });

                    if (it != rooms.end()) {
                        client.SetRoomId(0);    // 로비로 이동 (방 번호를 0으로 설정)
                        it->LeaveMember();      // 방의 멤버 수 감소
                        cout << "[시스템 메시지] " << client.GetName() << " 유저가 방 " << room_id << "을 떠났습니다." << endl;
                    } else {
                        // 방이 존재하지 않는 경우 처리
                        cerr << "존재하지 않는 방에서 나가려고 했습니다: " << room_id << endl;
                        return;
                    }
                    
                    break;
                }
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

    void ProcessNameCommand(Client &client, char* buffer, size_t real_data_bytes, uint16_t name_message_size){
        // 채팅 메시지 파싱
        CSName name_message;
        string system_message;
        if (!name_message.ParseFromArray(buffer + 2 + real_data_bytes, name_message_size)) {
            cerr << "Failed to parse CSName message." << endl;
        } else {
            // 메시지 처리 (예: 채팅 메시지 출력)
            client.SetName(name_message.name());

            system_message = "유저의 이름이 " + name_message.name() + " 으로 변경되었습니다.";
            cout << "[시스템 메시지] " << client.GetPort() << " " << system_message << endl;
        }

        SCSystemMessage system_message_data;
        system_message_data.set_text(system_message);

        Type type_message;
        type_message.set_type(Type::SC_SYSTEM_MESSAGE);

        string serialized_message;
        string serialized_type;

        // type,system 메시지를 직렬화
        type_message.SerializeToString(&serialized_type);
        system_message_data.SerializeToString(&serialized_message);

        // 메시지 길이 전송
        uint16_t message_length = htons(static_cast<uint16_t>(serialized_type.size()));
        send(client.GetSocket(), &message_length, sizeof(uint16_t), 0);

        // 직렬화된 타입 전송
        send(client.GetSocket(), serialized_type.c_str(), serialized_type.size(), 0);

        message_length = htons(static_cast<uint16_t>(serialized_message.size()));
        send(client.GetSocket(), &message_length, sizeof(uint16_t), 0);

        // 직렬화된 시스템 메시지 전송
        send(client.GetSocket(), serialized_message.c_str(), serialized_message.size(), 0);
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

    vector<Room> rooms{1000};   // 방 최대 갯수는 1000개
    int next_room_id = 0;
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