// #2: 동시에 열 수 있는 descriptor 수

#include <arpa/inet.h>
#include <errno.h>          // errno 라는 전역 변수를 쓰기 위함
#include <string.h>         // strerror() 라는 함수를 쓰기 위함
#include <sys/socket.h>

#include <unistd.h>
#include <iostream>

using namespace std;

int main(){
    for(int i=0; i < 1024; ++i){
        int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s < 0){
            cerr << "socket() failed: " << i << " " << strerror(errno) << endl;
            break;
        }
    }

    return 0;
}