// #8: 연결이 끊겼을 때 recv() 함수의 동작

#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>

#include <unistd.h>
#include <iostream>

using namespace std;

int main(){
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s < 0){
        cerr << "socket() failed: " << strerror(errno) << endl;
        return 1;
    }

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr("127.0.0.1");
    sin.sin_port = htons(10001);
    if(connect(s, (struct sockaddr *) &sin, sizeof(sin)) < 0){
        cerr << "connect() failed: " << strerror(errno) << endl;
        return 1;
    }

    char buf[1024];
    int r = send(s, buf, sizeof(buf), 0);
    if(r < 0){
        cerr << "send() failed: " << strerror(errno) << endl;
    } else{
        cout << "Sent: " << r << "bytes" << endl;
    }

    r = recv(s, buf, sizeof(buf), 0);
    if(r < 0){
        cerr << "recv() failed: " << strerror(errno) << endl;
    } else{
        cout << "Received: " << r << "bytes" << endl;
    }

    r = recv(s, buf, sizeof(buf), 0);
    if(r < 0){
        cerr << "recv() failed: " << strerror(errno) << endl;
    } else if(r == 0){          // test7.cpp 에서 추가 작성된 부분
        cout << "Socket closed" << endl;            // recv() 함수의 리턴 값이 0인 경우는 정상적인 경우로 본다. (연결하면 언젠가는 끊어지니까)
                                                    // recv() 함수는 연결이 끊길 경우 0을 반환
    } else{
        cout << "Received: " << r << "bytes" << endl;
    }

    close(s);
    return 0;
}