# 💻 채팅 서버
과제 설명: https://github.com/mjubackend/chat_client 
## 구현 화면
#### 클라이언트) /name 명령어 사용 + 서버에서 보낸 시스템 메시지 수신
![/name 명령어 사용 + 서버에서 보낸 시스템 메시지 수신](https://postfiles.pstatic.net/MjAyNDA1MDJfMjEx/MDAxNzE0NTg4MjY2MjMz.nzgyGxVoH_sPiEGVxnRq4x7DUmRjwCySuXAn7QQoT-kg.2Nk_6JDlKFFfmuY2ZnJVWh2xL6M_dDQPvqLsgP37f5Mg.PNG/%EA%B7%B8%EB%A6%BC1.png?type=w966)

#### 서버) /name 명령어 수신
![/name 명령어 수신](https://postfiles.pstatic.net/MjAyNDA1MDJfMTY2/MDAxNzE0NTg4MjY2MjI4.eP5Iwj48uXJdMCOcRqL1AdA0Iw3BiytHTVmtbHvpvEog.2lcApsTJ9wzAKuZP5cDycc3FPum0XKWWZhFxU5GEkfEg.PNG/%EA%B7%B8%EB%A6%BC2.png?type=w966)

#### 서버) 동시에 여러 클라이언트 접속
![서버) 여러 클라이언트 접속](https://postfiles.pstatic.net/MjAyNDA1MDJfMjM1/MDAxNzE0NTg4MjY2MjI4.GMfksyDJN0G1sv4pCg0lmbvSOIM4sx6B_0p04UDZX_Qg.VWgQ2tDn-k7oJwqk4hPLdQhzEY-_XXiiAURiVIoDEHMg.PNG/%EA%B7%B8%EB%A6%BC3.png?type=w966)

## 실행 방법
서버 먼저 실행해야 합니다.
```
$ g++ -o server server.cpp message.pb.cc -lprotobuf
$ sudo ./server [포트번호] [작업 쓰레드 갯수]
```

클라이언트 실행 방법입니다.
```
$ python3 ./client.py --format=protobuf --port=[포트번호] --verbosity=1
```

## 구현 요구 조건
* ✅ 멀티쓰레드 프로그래밍
* ✅ 직접 I/O multiplexing 구현 및 적용
* ✅ 직접 producer-consumer 구현 적용 (= queue, mutex, condition variable)
* ✅ 프레임워크 사용 불가
* ❌ `/name` 외 명령어 구현
* ❌ message handler map 적용

## 📑 구현 기능
* 클라이언트 ↔ 서버 채팅 가능
* 클라이언트 ↔ 시스템 메시지 송수신 가능
* `/name` 명령어 구현
