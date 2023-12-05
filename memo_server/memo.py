from http import HTTPStatus
import random
import requests
import json
import urllib

from flask import abort, Flask, make_response, render_template, Response, redirect, request
from flask_sqlalchemy import SQLAlchemy

app = Flask(__name__)

naver_client_id = 'SxSmUP2aYM6xCwaKDKGq'
naver_client_secret = 'Ffz7_NKevs'
naver_redirect_uri = 'http://localhost:8000/auth'

# MySQL 서버에 연결
app.config['SQLALCHEMY_DATABASE_URI'] = 'mysql+pymysql://user:0528@localhost/mysql'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
db = SQLAlchemy(app)

class User(db.Model):
    __tablename__ = 'user_db'
    user_id = db.Column(db.Integer, primary_key=True, autoincrement=True)
    user_name = db.Column(db.String(255))
    memo = db.Column(db.Text)

@app.route('/')
def home():
    # 쿠기를 통해 이전에 로그인 한 적이 있는지를 확인한다.
    # 이 부분이 동작하기 위해서는 OAuth 에서 access token 을 얻어낸 뒤
    # user profile REST api 를 통해 유저 정보를 얻어낸 뒤 'userId' 라는 cookie 를 지정해야 된다.
    # (참고: 아래 onOAuthAuthorizationCodeRedirected() 마지막 부분 response.set_cookie('userId', user_id) 참고)
    userId = request.cookies.get('userId', default=None)
    name = None

    ####################################################
    # TODO: 아래 부분을 채워 넣으시오.
    #       userId 로부터 DB 에서 사용자 이름을 얻어오는 코드를 여기에 작성해야 함
    if userId:
        # SQLAlchemy를 사용하여 데이터베이스에서 사용자 이름 가져오기
        user = User.query.filter_by(user_id=userId).first()
        name = user.user_name if user else None

    ####################################################

    # 이제 클라에게 전송해 줄 index.html 을 생성한다.
    # template 로부터 받아와서 name 변수 값만 교체해준다.
    return render_template('index.html', name=name)

# 로그인 버튼을 누른 경우 이 API 를 호출한다.
# OAuth flow 상 브라우저에서 해당 URL 을 바로 호출할 수도 있으나,
# 브라우저가 CORS (Cross-origin Resource Sharing) 제약 때문에 HTML 을 받아온 서버가 아닌 곳에
# HTTP request 를 보낼 수 없는 경우가 있다. (예: 크롬 브라우저)
# 이를 우회하기 위해서 브라우저가 호출할 URL 을 HTML 에 하드코딩하지 않고,
# 아래처럼 서버가 주는 URL 로 redirect 하는 것으로 처리한다.
#
# 주의! 아래 API 는 잘 동작하기 때문에 손대지 말 것
@app.route('/login')
def onLogin():
    params = {
        'response_type': 'code',
        'client_id': naver_client_id,
        'redirect_uri': naver_redirect_uri,
        'state': random.randint(0, 10000)
    }
    urlencoded = urllib.parse.urlencode(params)
    url = f'https://nid.naver.com/oauth2.0/authorize?{urlencoded}'
    return redirect(url)

# 아래는 Redirect URI 로 등록된 경우 호출된다.
# 만일 본인의 Redirect URI 가 http://localhost:8000/auth 의 경우처럼 /auth 대신 다른 것을
# 사용한다면 아래 @app.route('/auth') 의 내용을 그 URL 로 바꿀 것
@app.route('/auth')
def onOAuthAuthorizationCodeRedirected():
    # TODO: 아래 1 ~ 4 를 채워 넣으시오.

    # 1. redirect uri 를 호출한 request 로부터 authorization code 와 state 정보를 얻어낸다.
    authorization_code = request.args.get('code')  # code: 네이버 로그인 인증에 성공하면 반환받는 인증 코드, 접근 토큰(access token) 발급에 사용
    state = request.args.get('state')  # state: 사이트 간 요청 위조 공격을 방지하기 위해 애플리케이션에서 생성한 상태 토큰으로 URL 인코딩을 적용한 값

    # 2. authorization code 로부터 access token 을 얻어내는 네이버 API 를 호출한다.
    access_token_url = 'https://nid.naver.com/oauth2.0/token'  # 접근 토큰 발급/갱신/삭제 요청
    access_token_params = {
        'grant_type': 'authorization_code',  # 인증 과정에 대한 구분값 - 1. 발급:'authorization_code'
        'client_id': naver_client_id,  # 애플리케이션 등록 시 발급받은 Client ID 값
        'client_secret': naver_client_secret,  # 애플리케이션 등록 시 발급받은 Client secret 값
        'code': authorization_code,  # 로그인 인증 요청 API 호출에 성공하고 리턴받은 인증코드값 (authorization_code)
        'state': state
    }

    response = requests.post(access_token_url, data=access_token_params)

    # 3. 얻어낸 access token 을 이용해서 프로필 정보를 반환하는 API 를 호출하고,
    #    유저의 고유 식별 번호를 얻어낸다.
    if response.status_code == 200:
        token_info = response.json()
        # 'access_token' 키가 있는지 확인
        if 'access_token' in token_info:
            access_token = token_info['access_token']
        else:
            abort(404, 'Failed to get access token')

        profile_url = 'https://openapi.naver.com/v1/nid/me'  # 유저 프로필 데이터에 접근하는 기본 URL
        headers = {'Authorization': f'Bearer {access_token}'}  # 얻은 access token을 포함하는 헤더

        profile_response = requests.get(profile_url, headers=headers)  # 프로필 정보를 가져옴

        if profile_response.status_code == 200:
            user_info = profile_response.json().get('response', {})
            user_id = user_info.get('id')
            user_name = user_info.get('name')

            # 4. 얻어낸 user id 와 name 을 DB 에 저장한다.
            if user_id and user_name:
                try:
                    # SQLAlchemy를 사용하여 데이터베이스에 유저 정보 저장
                    user = User.query.filter_by(user_id=user_id).first()
                    if user:
                        user.user_name = user_name
                    else:
                        new_user = User(user_id=user_id, user_name=user_name)
                        db.session.add(new_user)
                    db.session.commit()
                except Exception as e:
                    print(f"Error: while saving user info to DB: {e}")
                    db.session.rollback()

                # 5. 첫 페이지로 redirect 하는데 로그인 쿠키를 설정하고 보내준다.
                response = redirect('/')
                response.set_cookie('userId', str(user_id))  # +
                print(response)
                print("성공!!")
                print(user_id)
                return response
            else:
                # user_id 또는 user_name이 없는 경우
                abort(404, 'Failed to get user id or user name')
        else:
            # 프로필 정보를 가져오는데 실패한 경우
            abort(profile_response.status_code)
    else:
        # Access Token을 가져오는데 실패한 경우
        abort(response.status_code)


@app.route('/memo', methods=['GET'])
def get_memos():
    # 로그인이 안되어 있다면 로그인 하도록 첫 페이지로 redirect 해준다.
    userId = request.cookies.get('userId', default=None)
    if not userId:
        return redirect('/')

    # TODO: DB 에서 해당 userId 의 메모들을 읽어오도록 아래를 수정한다.
    # + 이 함수의 return 형태대로 리턴하도록 수정
    result = []

    try:
        # SQLAlchemy를 사용하여 데이터베이스에서 메모들을 가져오기
        user = User.query.filter_by(user_id=userId).first()
        memos = user.memo.split('\n') if user and user.memo else []
        result = [{'content': memo} for memo in memos]
    except Exception as e:
        print(f"Error while fetching memos: {e}")

    # # memos라는 키 값으로 메모 목록 보내주기
    return Response(json.dumps({'memos': result}), content_type='application/json')


@app.route('/memo', methods=['POST'])
def post_new_memo():
    # 로그인이 안되어 있다면 로그인 하도록 첫 페이지로 redirect 해준다.
    userId = request.cookies.get('userId', default=None)
    if not userId:
        return redirect('/')

    # 클라이언트로부터 JSON 을 받았어야 한다.
    if not request.is_json:
        abort(HTTPStatus.BAD_REQUEST)

    # TODO: 클라이언트로부터 받은 JSON 에서 메모 내용을 추출한 후 DB에 userId 의 메모로 추가한다.
    try:
        memo_content = request.json.get('text', '')
        # SQLAlchemy를 사용하여 데이터베이스에 새로운 메모 추가
        user = User.query.filter_by(user_id=userId).first()
        if user:
            user.memo = (user.memo or "") + memo_content + '\n'
            db.session.commit()
    except Exception as e:
        print(f"Error while saving memo: {e}")
        db.session.rollback()
        return Response(json.dumps({'error': 'Failed to save memo'}), content_type='application/json'), HTTPStatus.INTERNAL_SERVER_ERROR

    #
    return '', HTTPStatus.OK

if __name__ == '__main__':
    app.run('0.0.0.0', port=8000, debug=True)
