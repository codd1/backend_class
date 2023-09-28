// #2: 재사용 되는 Socket Descriptor 값

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

using namespace std;

int main(){
    int s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    cout << "Socket ID: "<<s<<endl;
    close(s);

    s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    cout << "Socket ID: "<<s<<endl;
    close(s);

    // 값이 재사용 된다고 해서 "같은 소켓"이 아니다.
    // 우연히 값이 같아도 둘은 "완전히 다른 소켓"이다.
    // 따라서 socket()으로 얻어낸 것은 오직 1회만 close() 해야한다. (안 하면 소켓 자원 누수)
    return 0;
}