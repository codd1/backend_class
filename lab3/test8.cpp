// #8: UDP 서버 만들기 (echo server)

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

    char buf[65536];
    
    struct sockaddr_in servSin;
    struct sockaddr_in clientSin;

    memset(&servSin, 0, sizeof(servSin));
    servSin.sin_family = AF_INET;
    servSin.sin_addr.s_addr = INADDR_ANY;
    servSin.sin_port = htons(20000 + 176);
    if (bind(s, (struct sockaddr *) &servSin, sizeof(servSin)) < 0) {       // 내 socket의 port 번호를 명시하기 위해 bind() 함수 사용
                                                                    // 내 socket의 port 번호를 명시하지 않으면 -> 빈 소켓 번호 아무거나 배정됨
        cerr << strerror(errno) << endl;
        return 0;
    }
    // bind()는 client든 server든 관계 없이 할 수 있다.
    // 다만, 통상적으로 서버의 port가 미리 정해져야 client가 요청을 보낼 수 있으므로 '대개 서버 측에서는 bind()를 호출'한다.

    while(1){
        socklen_t client_sin_size = sizeof(clientSin);
        int numBytes = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr*)&servSin, &client_sin_size);
        if(numBytes <= 0){      // 에러인 경우(-1) 일부만 수신하는 경우는 없다.
            break;
        }

        numBytes = sendto(s, buf, numBytes, 0, (struct sockaddr*)&servSin, client_sin_size);    // recvfrom()한 내용을 그대로 sendto()
        if(numBytes <= 0){      // 에러인 경우(-1) 일부만 전송하는 경우는 없다.
            break;
        }
    }
    
    close(s);
    return 0;
}