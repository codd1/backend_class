# #9: JSON Serialize - 중첩된 JSON object
# 실행: python3 ./test9.py 

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
    print(s)

if __name__ == '__main__':
    main(sys.argv)