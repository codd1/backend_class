# 13. 멀티 쓰레드, 멀티 프로세스

from flask import Flask

app = Flask(__name__)

count = 0       # 전역 변수를 쓰는 코드 패턴 사용 X

@app.route('/increase')
def on_increase():
    global count        # stateful하다. (값을 기억하고 1씩 더해주니까)
    count += 1
    return str(count)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=10001)


"""
    Python Application Server 작성시 주의점

    Python application server는 WSGI를 통해서 실행되는데,
    WSGI가 몇 개의 processes/threads로 실행될지 모른다.

    각 요청은 서로 다른 WSGI process에 의해 실행되기 때문에
    ★ 전역 변수를 쓰는 코드 패턴을 써서는 안된다.

    즉, Application server에 어떤 state를 저장해서는 안된다. (정보는 외부 DB로)
"""