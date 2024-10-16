// #10: Condition Variable 사용

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

using namespace std;

int sum = 0;
mutex m;
condition_variable cv;

void f(){
    for(int i = 0; i < 10 * 1000 * 1000; ++i){
        ++sum;
    }
    {
        unique_lock<mutex> lg(m);
        cv.notify_one();
    }
}

int main(){
    thread t(f);
    {
        unique_lock<mutex> lg(m);
        cv.wait(lg);
        cout << "Sum: " << sum << endl;
    }
    t.join();
}