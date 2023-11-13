#1. Flask 이용한 Python App 작성

from flask import Flask

app = Flask(__name__)

@app.route('/')
def hello_world():
    return 'Hello, World!'

@app.route('/bad')
def hello_world1():
    return 'Bad World!'

@app.route('/good')
def hello_world2():
    return 'Good World!'

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=19176)
