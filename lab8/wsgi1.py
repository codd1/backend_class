# 11. WSGI가 기대하는 App Server Code 형태

def application(environ, start_response):       # Callable (첫 번째 인자는 dict)
    print(environ['REQUEST_METHOD'])
    print(environ['PATH_INFO'])

    status = '200 OK'
    headers = [('Content-Type', 'text/html')]

    start_response(status, headers)   # HTTP response body 반환 전 start_response() 호출

    return [b'Hello World']     # HTTP body는 iterable이어야 함


# $ uwsgi --socket 127.0.0.1:19176 --plugin /usr/lib/uwsgi/plugins/python3_plugin.so --module wsgi

# 브라우저 열고 http://sysdesignlab.mju.ac.kr/60202270 입력
# $ curl -X GET http://sysdesignlab.mju.ac.kr/60202270