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

#include "message.pb.h"

#define MAX_BUFFER_SIZE 65536

using namespace std;
using namespace mju;

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
        BindConnection(port);
    }

    ~Server() {
        for (int i = 0; i < num_thread_; i++) {
            threads[i].join();
        }
    }

    void Run() {
        for (int i = 0; i < num_thread_; i++) {
            threads.emplace_back(thread(&Server::WorkerThread, this));
        }

        while (!stop_flag) {
            fd_set read_fds = fd_set_;
            int active_socket_count = select(max_fd + 1, &read_fds, nullptr, nullptr, nullptr);

            if (active_socket_count < 0) {
                cerr << "select() failed: " << strerror(errno) << endl;
                exit(1);
            }

            if (FD_ISSET(passive_sock, &read_fds)) {
                AcceptConnection();
            }

            for (int i = 0; i < clients.size(); i++) {
                Client &client = clients[i];
                if (FD_ISSET(client.GetSocket(), &read_fds)) {
                    ReceiveData(client);
                }
            }
        }
    }

    void Stop() {
        stop_flag = true;
        cv.notify_one();
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

        if (listen(passive_sock, 10) < 0) {
            cerr << "listen() failed: " << strerror(errno) << endl;
            exit(1);
        }

        FD_ZERO(&fd_set_);
        FD_SET(passive_sock, &fd_set_);
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

        SetNonBlocking(client_sock);
        FD_SET(client_sock, &fd_set_);
        max_fd = max(max_fd, client_sock);

        {
            unique_lock<mutex> lock(m);
            clients.emplace_back(client_sock);
        }

        cout << "Socket " << client_sock << " 연결 완료" << endl;

        cv.notify_one();
    }

    void ReceiveData(Client &client) {
        int socket = client.GetSocket();
        vector<char> &incoming_data = client.GetReceivedData();

        char buffer[MAX_BUFFER_SIZE];
        ssize_t received_bytes = recv(socket, buffer, sizeof(buffer), 0);

        if (received_bytes <= 0) {
            if (received_bytes == 0) {
                cout << "Socket " << socket << " 연결 종료" << endl;
            } else {
                cerr << "recv() failed: " << strerror(errno) << endl;
                exit(1);
            }

            close(socket);
            FD_CLR(socket, &fd_set_);
            {
                unique_lock<mutex> lock(m);
                clients.erase(remove_if(clients.begin(), clients.end(), [socket](const Client &c) { return c.GetSocket() == socket; }), clients.end());
            }
        } else {
            incoming_data.insert(incoming_data.end(), buffer, buffer + received_bytes);
            ProcessData(client);
        }
    }

    void ProcessData(Client &client) {
        int socket = client.GetSocket();
        vector<char> &incoming_data = client.GetReceivedData();

        string message(incoming_data.begin(), incoming_data.end());
        cout << "Received from socket " << socket << ": " << message << endl;

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

    void WorkerThread() {
        while (true) {
            Client c;

            {
                unique_lock<mutex> lock(m);

                cv.wait(lock, [this] { return !clients.empty() || stop_flag; });

                if (stop_flag) {
                    break;
                }

                if (!clients.empty()) {
                    c = move(clients.front());
                    clients.erase(clients.begin());
                }
            }

            ProcessData(c);
        }
    }

private:
    int passive_sock;
    fd_set fd_set_;
    int max_fd;
    vector<Client> clients;

    int num_thread_;
    vector<thread> threads;

    mutex m;
    condition_variable cv;

    bool stop_flag;
};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        return 1;
    }

    int port = stoi(argv[1]);
    int num_thread = stoi(argv[2]);

    Server server(port, num_thread);

    server.Run();

    return 0;
}
