# 프로젝트 완성본 설명

## 1. Local 환경
### 📽️ 데모 영상
https://youtu.be/JCpwNZvg6sw


### 실행 방법
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

### 📑 구현 기능
* `http://localhost:8000` 접속 시 메모장 브라우저 접속 가능
* 네이버 로그인 버튼 클릭 시 네이버 ID, 비밀번호 입력 가능
* 로그인 성공 시 User의 메모장 정보가 나타납니다. 새로운 메모를 작성할 수 있고, 기존에 작성했던 메모들도 출력됩니다.
* 새로고침을 하거나, 화면을 닫은 후 다시 접속해도 로그인 정보가 남아있습니다.


## 2. AWS 인스턴스 내 환경

### 실행 방법
AWS 인스턴스 60202270-1, 60202270-2에 접속합니다. (ssh -i "60202270.pem" ubuntu@인스턴스IP)

docker mysql 서버 실행 방법입니다.
```
$ sudo docker-compose up
```

memo.py 실행 방법입니다.
```
$ python3 memo.py
```

### 📑 구현 기능
* AWS 인스턴스 60202270-1, 60202270-2 에서 `http://60202270-lb-1760040258.ap-northeast-2.elb.amazonaws.com`(로드밸런서 DNS) 로 메모장 브라우저 접속 가능
* 네이버 로그인 버튼 클릭 시 네이버 로그인 창 로드됨

# 구현하지 못한 요소
* AWS 인스턴스 내 환경에서 로그인 후 브라우저 접속 실패
    * 이후 User의 메모장 정보 알 수 없음


