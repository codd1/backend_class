// [미니 프로젝트] 채팅 서버 만들기

#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <unistd.h>
#include <iostream>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
//#include <vector>
#include <unordered_map>

#include "message.pb.h"

using namespace std;

struct UserInfo {
    string username;
    int roomID;
};

// 채팅 메시지 구조체
struct ChatMessage {
    string sender;
    string message;
    int roomID;
};

class ChatServer {
public:
    ChatServer(int numThreads) : numThreads_(numThreads) {
        // 초기화 작업
    }

    // 채팅방에 메시지 전송
    void sendMessage(const ChatMessage& message) {
        unique_lock<mutex> lock(mutex_);

        // 해당 대화방의 큐에 메시지 추가
        messageQueue_[message.roomID].push(message);
        cout << message.message << endl;

        // 대기 중인 쓰레드에 알림
        condition_.notify_one();    // wait() 이후 실행
    }

    // 사용자가 채팅방에 입장
    void joinRoom(const UserInfo& user) {
        unique_lock<mutex> lock(mutex_);

        // 사용자 정보 등록
        users_[user.username] = user;
    }

    // 메시지를 처리하는 스레드
    void messageHandler() {
        while (true) {
            unique_lock<mutex> lock(mutex_);

            // 큐가 비어있으면 대기
            while (messageQueue_.empty()) {
                condition_.wait(lock);      // 메인 thread 기다려
            }

            // 큐에서 메시지 꺼내기
            ChatMessage message = messageQueue_.begin()->second.front();
            messageQueue_.begin()->second.pop();

            lock.unlock();

            // 메시지 처리
            processMessage(message);
        }
    }

    // 메시지 처리 함수
    void processMessage(const ChatMessage& message) {
        // 실제 메시지 처리 로직을 여기에 구현
        cout << "Room " << message.roomID << ": " << message.sender << ": " << message.message << endl;
    }

    // 시작 함수
    void start() {
        // Worker Thread 생성
        /*
             ChatServer 클래스의 객체인 this에 대해 messageHandler 함수를 새로운 스레드에서 실행하도록 하고,
             이때 i + 1이 messageHandler 함수에 전달되어 스레드의 ID로 사용됩니다.
             스레드가 생성되면 threads_ 벡터에 해당 스레드가 추가됩니다.
        */
        for (int i = 0; i < numThreads_; ++i) {
            threads_.emplace_back([this, i]() { this->messageHandler(); });
            cout << "스레드 생성"<<endl;
        }
    }

    // 종료 함수
    void stop() {
        for (auto& thread : threads_) {
            thread.join();
            cout << "종료" << endl;
        }
    }

private:
    mutex mutex_;
    condition_variable condition_;
    unordered_map<string, UserInfo> users_;
    unordered_map<int, queue<ChatMessage>> messageQueue_;

    int numThreads_;    // 서버는 worker thread 들의 개수를 수정할 수 있어야 한다.
    vector<thread> threads_;
};


int main(){
    int passiveSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    int on = 1;
    setsockopt(passiveSock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(9176);
    if(bind(passiveSock, (struct sockaddr *)&sin, sizeof(sin)) < 0){
        cerr << "bind() failed: " << strerror(errno) << endl;
        return 1;
    }

    if(listen(passiveSock, 10) < 0){
        cerr << "listen() failed: " << strerror(errno) << endl;
        return 1;
    }

    while(1){
        memset(&sin, 0, sizeof(sin));
        unsigned int sin_len = sizeof(sin);
        int clientSock = accept(passiveSock, (struct sockaddr *)&sin, &sin_len);
        if(clientSock < 0){
            cerr << "accept() failed: " << strerror(errno) << endl;
            return 1;
        }

        char buf[65536];
        int numRecv = recv(clientSock, buf, sizeof(buf), 0);
        if(numRecv == 0){       // 닫힌 경우
            cout << "Socket closed: " << clientSock << endl;
        } else if(numRecv < 0){
            cerr << "recv() failed: " << strerror(errno) << endl;
        } else{
            //c->ParseFromString(string(buf, numRecv));
            cout << "Received: " << numRecv << "bytes, clientSock " << clientSock << endl;
        }

        int offset = 0;
        while(offset < numRecv){
            int numSend = send(clientSock, buf + offset, numRecv - offset, 0);
            if(numSend < 0){
                cerr << "send() failed: " << strerror(errno) << endl;
                return 1;
            } else{
                cout << "Sent: " << numSend << endl;
                offset += numSend;
            }
        }

        close(clientSock);
    }

    close(passiveSock);

}