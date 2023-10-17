// #6: UDP로 Protobuf 전송 (UDP echo server)
// 실행: $ g++ -o test6 test6.cpp person.pb.cc -lprotobuf

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <fstream>

#include "person.pb.h"

using namespace std;
using namespace mju;

int main(){
    Person *p = new Person;
    p->set_name("DK Moon");
    p->set_id(12345678);
    
    Person::PhoneNumber* phone = p->add_phones();       // 객체 생성 후 반환
    phone->set_number("010-111-1234");
    phone->set_type(Person::MOBILE);
    
    phone = p->add_phones();
    phone->set_number("02-100-1000");
    phone->set_type(Person::HOME);
    
    const string s = p->SerializeAsString();
    cout << "Length:" << s.length() << endl;
    cout << s << endl << endl;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sock<0)return 1;

    //string buf = "Hello World";

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(10001);
    sin.sin_addr.s_addr = inet_addr("127.0.0.1");

    int numBytes = sendto(sock, s.c_str(), s.length(), 0, (struct sockaddr*) &sin, sizeof(sin));
    cout << "Sent: " << numBytes << endl;

    char s2[65536];
    memset(&sin, 0, sizeof(sin));
    socklen_t sin_size = sizeof(sin);
    numBytes = recvfrom(sock, s2, sizeof(s2), 0, (struct sockaddr*) &sin, &sin_size);      // 보낸 사람의 주소를 담을 변수 크기
    cout << "Recevied: "<< numBytes << endl;
    cout << "From "<< inet_ntoa(sin.sin_addr) << endl;


    Person *p2 = new Person;        // 객체 생성
    p2->ParseFromString(string(s2, numBytes));
    cout << "Name:" << p2->name() << endl;
    cout << "ID:" << p2->id() << endl;
    for (int i = 0; i < p2->phones_size(); ++i) {
        cout << "Type:" << p2->phones(i).type() << endl;
        cout << "Phone:" << p2->phones(i).number() << endl;
    }

    close(sock);
    return 0;
}