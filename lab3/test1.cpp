// #1: Opaque Handle 로서의 Socket Descriptor

#include <arpa/inet.h>      // the Internet 관련 함수들
#include <sys/socket.h>     // socket 관련 기본 함수들
#include <unistd.h>         // Unix 계열에서 표준 기능 함수들

#include <iostream>

using namespace std;

int main(){
    int s =socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);    // Socket Descriptor
    cout << "Socket ID: "<<s<<endl;

    close(s);
    return 0;
}