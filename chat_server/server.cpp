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


class Room{
public:
    int room_id;
    string title;
    vector<string> members;
};

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

    Type& GetType(){
        return type_;
    }

    void SetRoomId(int id) {
        room_id = id;
    }

    int GetRoomId() const {
        return room_id;
    }

    // 채팅 서버에 있는 대화방에 참여중인 멤버의 이름 반환
    string GetReceivedDataAsString() const {
        return string(received_data.begin(), received_data.end());
    }

    vector<char>& GetReceivedData() {
        return received_data;
    }

private:
    int socket_;
    vector<char> received_data;
    int room_id;
    Type type_;
    Type_MessageType m_type_;
};

class Server {
public:
    Server(int port, int num_thread){
        num_thread_ = num_thread;
        stop_flag = false; 
        header_flag = true;
        type_flag = true;
        BindConnection(port);
    }

    ~Server() {
        for (int i = 0; i < num_thread_; i++) {
            threads[i].join();
        }
    }

    void Run() {
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
                cout << i << endl;
                Client &client = clients[i];
                //cout << "Client.GetSocket(): " << client.GetSocket() << endl;
                if(client.GetSocket() == passive_sock){
                    continue;
                }
                if (FD_ISSET(client.GetSocket(), &temp_rfds)) {
                    ReceiveData(client);
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
        //cout << "Socket " << client_sock << " 연결 완료" << endl;

        cv.notify_one();
    }

    void ReceiveData(Client &client) {
        Type typeMessage = client.GetType();

        if (type_flag){
            cout << "Type 먼저 들어옴 1111" << endl;
            ReceiveTypeData(client);

            typeMessage.set_type(mju::Type::CS_NAME);

            cout << typeMessage.GetTypeName() << endl;

            type_flag = false;
        } else{
            cout << "실제 데이터 들어옴 2222" << endl;
            cout << typeMessage.GetTypeName() << endl;
            ReceiveTypeData(client);

            type_flag = true;
        }
    }

    void RecvErrorHandling(int socket, ssize_t recv_bytes){
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
            exit(1);
        }
    }

    void ReceiveTypeData(Client &client){
        int socket = client.GetSocket();
        vector<char> &incoming_data = client.GetReceivedData();

        if (incoming_data.size() < sizeof(uint16_t)) {
            char header_buffer[sizeof(uint16_t)];
            ssize_t received_header_bytes = recv(socket, header_buffer, sizeof(header_buffer), 0);

            RecvErrorHandling(socket, received_header_bytes);

            incoming_data.insert(incoming_data.end(), header_buffer, header_buffer + sizeof(uint16_t));
        }

        uint16_t message_length;
        memcpy(&message_length, incoming_data.data(), sizeof(uint16_t));
        message_length = ntohs(message_length);

        while (incoming_data.size() < sizeof(uint16_t) + message_length) {
            char buffer[MAX_BUFFER_SIZE];
            ssize_t received_bytes = recv(socket, buffer, min(sizeof(buffer), static_cast<size_t>(message_length)), 0);

            RecvErrorHandling(socket, received_bytes);

            cout << "Received bytes: " << received_bytes << endl;
            incoming_data.insert(incoming_data.end(), buffer, buffer + received_bytes);
        }

        

        Type message;
        if (message.ParseFromArray(incoming_data.data() , message_length) + sizeof(uint16_t)) {
            cout << "파싱 성공" << endl;
            ProcessReceivedData(client, message);
            incoming_data.clear();
        } else {
            cout << "파싱 실패!!!" << endl;
            exit(1);
        }
        
    }

    void ProcessReceivedData(Client &client, const Type &message_type) {
        // 클라이언트로부터 수신한 데이터 처리
        //string received_data = client.GetReceivedDataAsString();

        switch (message_type.type()) {
            //cout << message_type.type() << endl;
            case Type::CS_NAME:
                cout << "name 진입" << endl;
                //ProcessNameCommand(client, message_type);
                break;
            case Type::CS_CHAT:
                //ProcessChatCommand(client, received_data);
                break;
            default:
                cerr << "Unknown message type." << message_type.type() << endl;
                break;
        }

        int socket = client.GetSocket();
        vector<char> &incoming_data = client.GetReceivedData();

        string message(incoming_data.begin(), incoming_data.end());

        message.erase(remove_if(message.begin(), message.end(), [](char c) { return isspace(static_cast<unsigned char>(c)); }), message.end());

        cout << message << endl;

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

            //ProcessReceivedData(c);       //?
        }
    }

    void ProcessNameCommand(Client &client, const Type &message) {
        CSName cs_name;
        vector<char> &incoming_data = client.GetReceivedData();

        string received_data(incoming_data.begin(), incoming_data.end());

        if (cs_name.ParseFromString(received_data)) {
            string new_name = cs_name.name();
            cout << "현재 설정된 네임은: " << new_name << endl;
            string old_name = client.GetReceivedData().empty() ? "이름 없음" : string(client.GetReceivedData().begin(), client.GetReceivedData().end());

            client.GetReceivedData().clear();
            client.GetReceivedData().insert(client.GetReceivedData().end(), new_name.begin(), new_name.end());

            string system_message;
            if (client.GetReceivedData().empty()) {
                system_message = "[시스템 메시지] 이름이 변경되었습니다.";
            } else {
                system_message = "[시스템 메시지] 이름이 " + old_name + "에서 " + new_name + "으로 변경되었습니다.";
            }

            SendSystemMessage(client, system_message);
        } else {
            cerr << "Failed to parse CSName message." << endl;
            // 오류 처리를 수행하거나 연결을 종료하도록 수정할 수 있습니다.
        }
    }

    void processRoomsCommand(Client& client) {
        // Prepare SCRoomsResult message
        SCRoomsResult roomsResult;

        // Check if there are available rooms
        if (rooms.empty()) {
            // No rooms available
            cout << "No available rooms." << endl;
        } else {
            // Add room information to SCRoomsResult message
            for (const auto& room : rooms) {
                mju::SCRoomsResult::RoomInfo* roomInfo = roomsResult.add_rooms();
                roomInfo->set_roomid(room.room_id);
                roomInfo->set_title(room.title);

                // Add member names
                for (const auto& member : room.members) {
                    roomInfo->add_members(member);
                }
            }
        }

        // Serialize the message
        string serializedMessage;
        if (roomsResult.SerializeToString(&serializedMessage)) {
            // Send the message to the client
            SendProtobufMessage(client, mju::Type::SC_ROOMS_RESULT, serializedMessage);
        } else {
            cerr << "Error serializing SCRoomsResult message." << endl;
        }
    }

    void SendData(Client &client) {
        int socket = client.GetSocket();
        vector<char> &outgoing_data = client.GetReceivedData();

        ssize_t sent_bytes = send(socket, outgoing_data.data(), outgoing_data.size(), 0);

        if (sent_bytes <= 0) {
            cerr << "send() failed: " << strerror(errno) << endl;
        } else {
            // 성공적으로 보냈다면, 보낸 데이터 삭제
            outgoing_data.erase(outgoing_data.begin(), outgoing_data.begin() + sent_bytes);
        }
    }

    void SendProtobufMessage(Client &client, Type::MessageType message_type, const string &message) {
        Type type_message;
        type_message.set_type(message_type);
        //type_message.set_data(message);

        string serialized_type;
        type_message.SerializeToString(&serialized_type);

        uint16_t message_length = htons(serialized_type.length());
        string serialized_message(reinterpret_cast<char*>(&message_length), sizeof(uint16_t));
        serialized_message += serialized_type;

        client.GetReceivedData().insert(client.GetReceivedData().end(), serialized_message.begin(), serialized_message.end());

        SendData(client);
    }

    void SendSystemMessage(Client &client, const string &message) {
        SCSystemMessage sc_system_message;
        sc_system_message.set_text(message);

        string serialized_message;
        sc_system_message.SerializeToString(&serialized_message);

        SendProtobufMessage(client, Type::SC_SYSTEM_MESSAGE, serialized_message);
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

    bool stop_flag;
    bool header_flag;
    bool type_flag;

    vector<Room> rooms;
    int next_room_id = 1;   // room_id는 0부터 시작
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
    server.Run();

    return 0;
}