# #11: JSON Deserialize
# 실행: python3 ./test11.py 

import json
import sys

def main(argv):
    obj1 = {
        'name': 'DK Moon',
        'id': 12345678,
        'work': {               # JSON object 안에 value로서 다른 JSON object가 들어갈 수 있다.
            'name': 'Myongji Unviversity',
            'address': '116 Myongji-ro'
        }
    }

    s = json.dumps(obj1, indent=2)
    obj2 = json.loads(s)                # JSON 문자열로부터 python dict 객체를 생성한다. (= deserialize한다)
    print(obj2['name'], obj2['id'], obj2['work']['address'])
    print(obj1 == obj2)

if __name__ == '__main__':
    main(sys.argv)