// #9: recv() 가 일부 데이터만 반환할 때

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

    char buf[60 * 1024];        // test8.cpp 에서 변경된 부분
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
    } else if(r == 0){
        cout << "Socket closed" << endl;
    } else{
        cout << "Received: " << r << "bytes" << endl;
    }

    close(s);
    return 0;
}