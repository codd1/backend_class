// #3: C++에서 Serialize 해보기

#include <fstream>
#include <string>
#include <iostream>

#include "person.pb.h"

using namespace std;
using namespace mju;

int main()
{
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
    cout << s << endl;
}