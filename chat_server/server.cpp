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

#include "message.pb.h"

#define MAX_BUFFER_SIZE 100

using namespace std;
using namespace mju;


string client_name;

class Client {
public:
    Client(){
        socket_ = -1;
    }

    Client(int socket){
        socket_ = socket;
    }

    int GetSocket() const {
        return socket_;
    }

    vector<char>& GetReceivedData() {
        return received_data;
    }

private:
    int socket_;
    vector<char> received_data;
};

class Server {
public:
    Server(int port, int num_thread){
        num_thread_ = num_thread;
        stop_flag = false; 
        type_flag = true;
        command_flag = true;
        BindConnection(port);
    }

    ~Server() {
        for (int i = 0; i < num_thread_; i++) {
            threads[i].join();
        }
    }

    void Run(Type* t) {
        for (int i = 0; i < num_thread_; i++) {
            // 스레드 생성
            cout << "메시지 작업 쓰레드 #" << i << " 생성" << endl;
            threads.emplace_back(thread([this] { WorkerThread(); }));
        }

        while (1) {
            if (stop_flag){
                break;
            }

            temp_rfds = rfds;
            
            int active_socket_count = select(max_fd + 1, &temp_rfds, NULL, NULL, NULL);

            if (active_socket_count <= 0) {     // 0인 경우는 timeout인 경우
                cerr << "select() failed: " << strerror(errno) << endl;
                break;
            }
            // 소켓이 읽기 가능한 상태
            // 새로운 클라이언트가 서버에 연결을 시도하면 select 함수가 수동 소켓에 대한 읽기 이벤트를 감지
            if (FD_ISSET(passive_sock, &temp_rfds)) {
                AcceptConnection();
            }

            for (int i = 0; i < active_socket_count; i++) {
                Client &client = clients[i];
                cout << "Client.GetSocket(): " << client.GetSocket() << endl;
                //cout << i << endl;
                if(client.GetSocket() == passive_sock){
                    continue;
                }
                if (FD_ISSET(client.GetSocket(), &temp_rfds)) {
                    ReceiveData(client, &type_);
                    ReceiveData(client, t);
                }
                cout << "-------------------" << endl;
            }
            if(command_flag){
                command_flag = false;
            } else{
                command_flag = true;
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

        // 클라이언트의 IP 주소와 port 주소 얻기 (데모 서버처럼 출력하기 위함)
        inet_ntop(AF_INET, &(client_sin.sin_addr), client_ip, INET_ADDRSTRLEN);
        client_port = ntohs(client_sin.sin_port);

        SetNonBlocking(client_sock);
        FD_SET(client_sock, &rfds);
        max_fd = max(max_fd, client_sock);

        {
            unique_lock<mutex> lock(m);
            clients.emplace_back(client_sock);
        }

        cout << "새로운 클라이언트 접속 [('" << client_ip << "', " << client_port << ")]" << endl;

        cv.notify_one();
    }

    void ReceiveData(Client &client, Type* t) {
        int socket = client.GetSocket();
        vector<char> &incoming_data = client.GetReceivedData();

        if (incoming_data.size() < sizeof(uint16_t)) {
            char header_buffer[sizeof(uint16_t)];
            ssize_t received_header_bytes = recv(socket, header_buffer, sizeof(header_buffer), 0);

            if(RecvErrorHandling(socket, received_header_bytes)){
                //incoming_data.clear();
                return;
            }

            incoming_data.insert(incoming_data.end(), header_buffer, header_buffer + sizeof(uint16_t));
        }

        uint16_t message_length;
        memcpy(&message_length, incoming_data.data(), sizeof(uint16_t));
        message_length = ntohs(message_length);

        while (incoming_data.size() < sizeof(uint16_t) + message_length) {
            char buffer[MAX_BUFFER_SIZE];
            ssize_t received_bytes = recv(socket, buffer, min(sizeof(buffer), static_cast<size_t>(message_length)), 0);

            if(RecvErrorHandling(socket, received_bytes)){
                //incoming_data.clear();
                return;
            }

            //cout << "Received bytes: " << received_bytes << endl;
            incoming_data.insert(incoming_data.end(), buffer, buffer + received_bytes);
        }

        Type message;
        if (message.ParseFromArray(incoming_data.data() , message_length) + sizeof(uint16_t)) {
            if(type_flag){
                t->set_type(Type::CS_NAME);
                type_flag = false;
                return;
            } else {
                ProcessReceivedData(client, message, t);
                incoming_data.clear();
                type_flag = true;
            }
        } else {
            //cout << "파싱 실패!!!" << endl;
            exit(1);
        }
        
    }

    bool RecvErrorHandling(int socket, ssize_t recv_bytes){
        if (recv_bytes <= 0) {
            if (recv_bytes == 0) {
                cout << "클라이언트 [('" << client_ip << "', " << client_port << "):" << " ]: 상대방이 소켓을 닫았음" << endl;
            } else {
                cerr << "recv() header failed: " << strerror(errno) << endl;
            }

            close(socket);
            FD_CLR(socket, &rfds);
            {
                unique_lock<mutex> lock(m);
                clients.erase(remove_if(clients.begin(), clients.end(), [socket](const Client &c) { return c.GetSocket() == socket; }), clients.end());
            }
            return true;
        } else{
            return false;
        }
    }

    void ProcessReceivedData(Client &client, const Type &type, const Type* t) {
        // 클라이언트로부터 수신한 데이터 처리
        //cout << message_type.GetTypeName() << endl;
        Type::MessageType messageType = type.type();
        cout << "Message Type: " << MessageTypeToString(messageType) << std::endl;

        switch (messageType) {
            case Type::CS_NAME:
                if(command_flag){
                    cout << "name 진입" << endl;
                    ProcessNameCommand(client, type);
                }
                break;
            case Type::CS_CHAT:
                //ProcessChatCommand(client, received_data);
                break;
            default:
                cerr << "Unknown message type." << messageType << endl;
                break;
        }

        vector<char> &incoming_data = client.GetReceivedData();
        string message(incoming_data.begin(), incoming_data.end());
        message.erase(remove_if(message.begin(), message.end(), [](char c) { return isspace(static_cast<unsigned char>(c)); }), message.end());
        incoming_data.clear();
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

    // 스레드가 실행할 함수
    void WorkerThread() {
        while (true) {
            Client c;

            {
                unique_lock<mutex> lock(m);
                
                // 스레드 대기 상태(wait) -> 대기 중인 클라이언트가 없고 stop flag가 설정되었을 때 스레드 종료
                cv.wait(lock, [this] { return !clients.empty() || stop_flag; });

                if (stop_flag) {
                    break;
                }

                // 대기 중인 클라이언트가 있을 때 클라이언트를 가져오고 리스트에서 제거
                if (!clients.empty()) {
                    c = move(clients.front());
                    clients.erase(clients.begin());
                }
            }

            // 클라이언트 소켓이 이미 닫힌 경우에는 처리를 건너뜀
            if (c.GetSocket() < 0) {
                continue;
            }
        }
    }

    void ProcessNameCommand(Client &client, const Type &message) {
        vector<char> &incoming_data = client.GetReceivedData();

        string received_data(incoming_data.begin(), incoming_data.end());

        string system_message;
        
        auto line_end_check = [](char c){
            return c == '\n' || c == 'r';
        };

        received_data.erase(remove_if(received_data.begin(), received_data.end(), line_end_check), received_data.end());
        client_name = received_data;

        system_message = "이름이 " + client_name + " 으로 변경되었습니다.";
        cout << "[시스템 메시지] " << system_message << endl;

        Type::MessageType messageType = message.type();
        //cout << "Message Type: " << MessageTypeToString(messageType) << std::endl;

        SendSystemMessage(client, system_message);

        //incoming_data.clear();
    }

    void SendSystemMessage(Client &client, const string &system_message) {
        // SCSystemMessage 객체 생성
        SCSystemMessage system_message_data;
        system_message_data.set_text(system_message);

        Type type_message;
        type_message.set_type(Type::SC_SYSTEM_MESSAGE);

        SendMessage(client, type_message, system_message_data);
    }

    // S->C
    void SendMessage(Client &client, const Type& type_message, const SCSystemMessage &system_message) {
        // 메시지를 직렬화
        string serialized_message;
        string serialized_type;

        // type,system 메시지를 직렬화
        type_message.SerializeToString(&serialized_type);
        system_message.SerializeToString(&serialized_message);

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

    // MessageType에 대응하는 문자열을 반환하는 함수
    const char* MessageTypeToString(Type::MessageType messageType) {
        switch (messageType) {
            case Type::CS_NAME: return "CS_NAME";
            case Type::CS_ROOMS: return "CS_ROOMS";
            case Type::CS_CREATE_ROOM: return "CS_CREATE_ROOM";
            case Type::CS_JOIN_ROOM: return "CS_JOIN_ROOM";
            case Type::CS_LEAVE_ROOM: return "CS_LEAVE_ROOM";
            case Type::CS_CHAT: return "CS_CHAT";
            case Type::CS_SHUTDOWN: return "CS_SHUTDOWN";
            case Type::SC_ROOMS_RESULT: return "SC_ROOMS_RESULT";
            case Type::SC_CHAT: return "SC_CHAT";
            case Type::SC_SYSTEM_MESSAGE: return "SC_SYSTEM_MESSAGE";
            default: return "Unknown";
        }
    }

private:
    int passive_sock;
    fd_set rfds, temp_rfds;
    int max_fd;
    Type type_;

    vector<Client> clients;
    char client_ip[20];
    int client_port;

    int num_thread_;
    vector<thread> threads;

    mutex m;
    condition_variable cv;

    bool stop_flag;
    bool type_flag;     // 타입 정보를 포함하는 메시지인지 (먼저 오는 메시지)
    bool command_flag;  // 명령어 정보를 포함하는 메시지인지 (두 번째로 오는 메시지)

    int next_room_id = 1;   // room_id는 0부터 시작

    SCSystemMessage system_message_;
};

int main(int argc, char *argv[]) {
    // g++ -o server server.cpp message.pb.cc -lprotobuf
    // ./server 9176 3
    if (argc != 3) {
        return 1;
    }

    int port = stoi(argv[1]);
    int num_thread = stoi(argv[2]);

    Server server(port, num_thread);
    Type *t = new Type;
    server.Run(t);

    delete(t);
    return 0;
}