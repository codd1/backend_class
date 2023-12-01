# 12. HTTP Request body 읽기

import json

def application(environ, start_response):       # Callable (첫 번째 인자는 dict)
    print(environ['REQUEST_METHOD'])
    print(environ['PATH_INFO'])

    body_bytes = environ['wsgi.input'].read()
    body_str = body_bytes.decode('utf-8')
    body_json = json.loads(body_str)

    status = '200 OK'
    headers = [('Content-Type', 'text/html')]

    start_response(status, headers)   # HTTP response body 반환 전 start_response() 호출

    response = f'Hello World {body_json["name"]}'
    return [bytes(response, encoding='utf-8')]     # HTTP body는 iterable이어야 함

# $ uwsgi --ini uwsgi.ini --module wsgi2

# $ curl -X POST http://localhost/60202270 -H "Content-Type: application/json; charset=utf-8" --data '{"name":"홍채영"}'