# #7: JSON Serialize 기초

import json
import sys

def main(argv):
    obj1 = {
        'name': 'DK Moon',
        'id': 12345678,
    }

    s = json.dumps(obj1)        # JSON 문자열로 serialize 하는 코드
    print(s)
    print('Type', type(s).__name__)

if __name__ == '__main__':
    main(sys.argv)