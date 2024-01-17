# 💻 2023년 2학기 백엔드소프트웨어개발 Repo
### 개인 정보
------------------------------------------
- 이름: 홍채영
- 학번: 60202270
___
### [미니프로젝트1] 채팅 서버 만들기
과제 설명: https://github.com/mjubackend/chat_client


### [미니프로젝트2] 메모장 서비스 만들기
과제 설명: https://github.com/mjubackend/memo_server

#### 1. 메모장 서비스 프로젝트 완성본 설명 (local 환경)

local 환경에서 구현한 메모장은 `메모장실행_local환경.mp4` 영상을 통해서도 확인할 수 있습니다.

docker mysql 서버 실행 방법입니다.
```
$ sudo docker-compose up
```

memo.py 실행 방법입니다.
```
$ python memo.py
```

만약 flask_sqlalchemy 가 설치되어 있지 않다면 아래와 같이 flask_sqlalchemy를 설치해줍니다.
```
$ pip install flask_sqlalchemy
```

* `http://localhost:8000` 접속 시 메모장 브라우저 접속 가능
* 네이버 로그인 버튼 클릭 시 네이버 ID, 비밀번호 입력 가능
* 로그인 성공 시 User의 메모장 정보가 나타납니다. 새로운 메모를 작성할 수 있고, 기존에 작성했던 메모들도 출력됩니다.
* 새로고침을 하거나, 화면을 닫은 후 다시 접속해도 로그인 정보가 남아있습니다.

#### 2. 메모장 서비스 프로젝트 완성본 설명 (AWS 인스턴스 내 환경)

AWS 인스턴스 60202270-1, 60202270-2에 접속합니다. (ssh -i "60202270.pem" ubuntu@인스턴스IP)

docker mysql 서버 실행 방법입니다.
```
$ sudo docker-compose up
```

memo.py 실행 방법입니다.
```
$ python3 memo.py
```
* AWS 인스턴스 60202270-1, 60202270-2 에서 `http://60202270-lb-1760040258.ap-northeast-2.elb.amazonaws.com`(로드밸런서 DNS) 로 메모장 브라우저 접속 가능
* 네이버 로그인 버튼 클릭 시 네이버 ID, 비밀번호 입력 가능

#### 3. 구현하지 못한 요소
* AWS 인스턴스 내 환경에서 로그인 후 브라우저 접속 실패
* 이후 User의 메모장 정보 알 수 없음


