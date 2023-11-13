#10. 계산기 RESTful API

"""
GET 메서드
    - $ curl -X GET http://localhost:19176/숫자1/연산자/숫자2
POST 메서드
    - $ curl -X POST http://localhost:19176/ -H "Content-Type: application/json; charset=utf-8" --data '{"arg1":숫자1, "op":"연산자", "arg2":숫자2}'
"""

from flask import Flask
from flask import request
from flask import make_response

app = Flask(__name__)

@app.route('/', methods=['POST'])
def post():
    if not all(key in request.get_json() for key in ['arg1', 'op', 'arg2']):
        return make_response('Bad Request', 400)
    
    num1 = request.get_json().get('arg1')
    operator = request.get_json().get('op')
    num2 = request.get_json().get('arg2')

    if operator not in ['+', '-', '*']:
        return make_response('400 Bad Request', 400)

    result = calcurator(num1, operator, num2)
    
    return {'result':result, 'response':'200 OK'}

@app.route('/<num1>/<operator>/<num2>', methods=['GET'])
def get(num1, operator, num2):
    if operator not in ['+', '-', '*']:
        return make_response('Bad Request', 400)
    
    num1 = int(num1)
    num2 = int(num2)

    result = calcurator(num1, operator, num2)

    return {'result':result, 'response':'200 OK'}

def calcurator(num1, operator, num2):
    if operator == '+':
        return num1 + num2
    elif operator == '-':
        return num1 - num2
    elif operator == '*':
        return num1 * num2

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=19176)
