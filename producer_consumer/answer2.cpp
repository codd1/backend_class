#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <thread>

using namespace std;

const int NO_JOB = -1;

// 쓰레드의 종료 조건을 조율하기 위한 flag.
// boolean 이라고 하더라도 atomic 하지 않기 때문에 atomic<bool> 을 이용한다.
atomic<bool> quit(false);

// 공유 자원인 queue.
// 공유 자원이 있다면 이를 보호하기 위한 mutex 가 있어야 한다.
// 그리고 공유 자원의 상태(condition) 에 변화가 생기는 것은 condition_variable 로 통신한다.
// condition_variable 은 mutex 가 반드시 필요하다.
// 그런데 여러개의 condition_variable 이 mutex 하나를 쓰는 지금 상황에 주목할 것.
mutex queMutex;
condition_variable queFillable;
condition_variable queFilled;
int que[1] = { NO_JOB };


// producer thread 의 thread 함수
void producer() {
  cout << "Producer starting. Thread id: " << this_thread::get_id() << endl;

  // thread 종료 flag 가 켜질 때가지 동작 시킨다.
  while (quit.load() == false) {
    int job = rand() % 100;
    if (que[0] == NO_JOB) {
      que[0] = job;
    }
  }
  cout << "Producer finished" << endl;
}


// consumer thread 의 thread 함수
void consumer() {
  cout << "Consumer starting. Thread id: " << this_thread::get_id() << endl;

  // thread 종료 flag 가 켜질 때까지 동작 시킨다.
  while (quit.load() == false) {
    int job;
    job = que[0];
    que[0] = NO_JOB;
  }
  cout << "Consumer finshed" << endl;
}


int main() {
  cout << "Main thread started. Thread id: " << this_thread::get_id() << endl;

  // 랜덤 생성기의 초기값을 지정한다.
  srand(time(NULL));

  // producer/consumer 쓰레드의 핸들을 저장할 C++ 측 객체
  thread t1(producer);
  thread t2(consumer);

  this_thread::sleep_for(chrono::seconds(5));

  // 쓰레드들을 종료시키도록 flag 를 켠다.
  quit.store(true);

  // thread.joinable() 은 C++ 측 thread 객체에 대응되는 OS 측 thread 가 있는지를 확인하는 것이다.
  // OS 측 thread 가 만들어지지 않았거나, 이미 thread 가 join 되었거나,
  // 아니면 우리가 다루지는 않았지만, OS thread 가 detach 된 경우 joinable() 은 false 를 반환한다.
  // 여기서는 쓰레드가 만들어진 경우만 join() 을 호출하기 위해서 사용한다.
  if (t1.joinable()) {
    t1.join();
  }

  // thread.joinable() 은 C++ 측 thread 객체에 대응되는 OS 측 thread 가 있는지를 확인하는 것이다.
  // OS 측 thread 가 만들어지지 않았거나, 이미 thread 가 join 되었거나,
  // 아니면 우리가 다루지는 않았지만, OS thread 가 detach 된 경우 joinable() 은 false 를 반환한다.
  // 여기서는 쓰레드가 만들어진 경우만 join() 을 호출하기 위해서 사용한다.
  if (t2.joinable()) {
    t2.join();
  }

  cout << "Main thread finished" << endl;
}
