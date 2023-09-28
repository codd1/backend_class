// #3: UDP 소켓으로 데이터 보내기

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
    
    struct sockaddr_in sin;     // IPv4 -> struct sockaddr_in
                                // IPv6 -> struct sockaddr_in6
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(10000);
    sin.sin_addr.s_addr = inet_addr("127.0.0.1");

    // sendto() 함수는 struct sockaddr*을 매개변수로 받는다.
    // 하지만 struct sockaddr_in을 사용하므로 강제로 형변환을 하고, 전체 몇 바이트까지 확보된 것인지를 같이 넘겨준다.
    int numBytes = sendto(s, buf.c_str(), buf.length(), 0, (struct sockaddr*) &sin, sizeof(sin));
    
    // UDP - sendto()가 반환값을 내놨다고 상대가 받았다고 확신하면 안된다. (Connectionless, Best-Effort)
    cout << "Sent: "<< numBytes << endl;

    close(s);
    return 0;
}