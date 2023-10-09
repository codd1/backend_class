# #10: JSON Serialize - 중첩된 Array

import json
import sys

def main(argv):
    obj1 = {
        'name': 'DK Moon',
        'id': 12345678,
        'work': {
            'name': 'Myongji Unviversity',
            'address': '116 Myongji-ro'
        },
        'scores': [100, 90.5, 80.0]         # scores라는 key 추가, value는 배열
                                            # 배열의 구성 요소가 같은 타입일 필요 X (정수, 소수)
    }

    s = json.dumps(obj1, indent=2)
    print(s)

if __name__ == '__main__':
    main(sys.argv)





