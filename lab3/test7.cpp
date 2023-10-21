// #7: 내 Socket이 쓸 Port 번호 지정

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <string>

using namespace std;

int main(){
    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(s<0)return 1;

    string buf = "Hello World";
    
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(10176);
    sin.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (bind(s, (struct sockaddr *) &sin, sizeof(sin)) < 0) {       // 내 socket의 port 번호를 명시하기 위해 bind() 함수 사용
                                                                    // 내 socket의 port 번호를 명시하지 않으면 -> 빈 소켓 번호 아무거나 배정됨
        cerr << strerror(errno) << endl;
        return 0;
    }
    // bind()는 client든 server든 관계 없이 할 수 있다.
    // 다만, 통상적으로 서버의 port가 미리 정해져야 client가 요청을 보낼 수 있으므로 '대개 서버 측에서는 bind()를 호출'한다.

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(20176);
    sin.sin_addr.s_addr = inet_addr("127.0.0.1");

    int numBytes = sendto(s, buf.c_str(), buf.length(), 0, (struct sockaddr*) &sin, sizeof(sin));
    cout << "Sent: "<<numBytes<<endl;

    char buf2[65536];
    memset(&sin, 0, sizeof(sin));
    socklen_t sin_size = sizeof(sin);
    numBytes = recvfrom(s, buf2, sizeof(buf2), 0, (struct sockaddr*) &sin, &sin_size);

    cout << "From " << inet_ntoa(sin.sin_addr) << endl;
    memset(&sin, 0, sizeof(sin));
    sin_size = sizeof(sin);
    int result = getsockname(s, (struct sockaddr *) &sin, &sin_size);
    if (result == 0) {
        cout << "My addr: " << inet_ntoa(sin.sin_addr) << endl;
        cout << "My port: " << ntohs(sin.sin_port) << endl;         // 내 socket의 port 번호를 명시했으므로 같은 port 번호가 출력된다.
    }
    
    close(s);
    return 0;
}