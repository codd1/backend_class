# #12: JSON을 UDP로 전송
# 실행: python3 ./test12.py 

import json
import sys
import socket

def main(argv):
    obj1 = {
        'name': 'DK Moon',
        'id': 12345678,
        'work': {               # JSON object 안에 value로서 다른 JSON object가 들어갈 수 있다.
            'name': 'Myongji Unviversity',
            'address': '116 Myongji-ro'
        }
    }

    s = json.dumps(obj1, indent=2)      # dumps() 함수: python 객체를 JSON 문자열로 변환 (= serialize한다)

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0)
    sock.sendto(bytes(s, encoding = 'utf-8'), ('127.0.0.1', 10001))     # bytes()는 객체 형태의 문자열을 메모리에 씌여지는 byte 형태로 변경하는 함수
    (data, sender) = sock.recvfrom(65536)

    obj2 = json.loads(data)                # UDP server로부터 돌려 받은 데이터 data로부터 obj2 생성 (= deserialize한다)
    print(obj2['name'], obj2['id'], obj2['work']['address'])
    print(obj1 == obj2)

if __name__ == '__main__':
    main(sys.argv)