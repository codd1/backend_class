// #4: 연결을 닫은 소켓에 데이터 쓰기

#include <arpa/inet.h>
#include <errno.h>          // errno 라는 전역 변수를 쓰기 위함
#include <string.h>         // strerror() 라는 함수를 쓰기 위함
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

    close(s);       // socket을 만들고 close한 뒤 send -> Bad file descriptor (존재하지 않는 디스크립터)

    char buf[1024];
    int r = send(s, buf, sizeof(buf), MSG_NOSIGNAL);        // MSG_NOSIGNAL는 flag이다. flag를 0으로 설정하면 오류 메시지 없이 그냥 종료된다.
    if(r < 0){
        cerr << "send() failed: " << strerror(errno) << endl;
    } else{
        cout << "Sent: " << r << "bytes" << endl;
    }

    close(s);
    return 0;
}