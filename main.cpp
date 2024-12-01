
#include "minheap.h"

#include <cstdint>
#include <iostream>
#include <functional>
#include <chrono>
#include <ctime>
#include <thread>

extern "C"{
#define offsetofs(s,m) (size_t)(&reinterpret_cast<const volatile char&>((((s*)0)->m)))
#define container_of(ptr, type, member) ({                  \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetofs(type, member) );})
}

using CallBack = std::function<void(void)>;

struct TimeNode {
    timer_entry env;
    CallBack cb;
};

class Timer {
public:
    Timer(uint32_t size);
    Timer() = delete;
    ~Timer();

    int addTimer(uint64_t time, CallBack cb);

    void run();

    void stop();

private:
    uint64_t GetNowTime();
private:
    min_heap* _heap;
    bool _close;
};

Timer::Timer(uint32_t size) {
    min_heap_init(&_heap, size);
    _close = false;
}

Timer::~Timer() {
    min_heap_free(_heap);
}

int Timer::addTimer(uint64_t time, CallBack cb) {
    TimeNode* node = new TimeNode();
    node->env.time = GetNowTime() + time;
    node->cb = cb;
    min_heap_push(_heap, &node->env);
    return 0;
}

uint64_t Timer::GetNowTime() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    uint64_t timestamp_ms = duration.count();
    return timestamp_ms;
}

void Timer::run() {
    while(!_close) {
        uint64_t now = GetNowTime();
        timer_entry* entry = nullptr;
        if(!min_heap_top(_heap, &entry)) {
            if(entry->time <= now) {
                if(!min_heap_pop(_heap, &entry)) {
                    TimeNode* node = container_of(entry, TimeNode, env);
                    if(node) {
                        node->cb();
                    }

                    delete node;
                }
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(entry->time - now));
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void Timer::stop() {
    _close = true;
}

int main(int, char**){

    Timer timer(10);

    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    uint64_t timestamp_ms = duration.count();
    std::cout << "Start, now time :" << timestamp_ms << std::endl;
    timer.addTimer(1000, [&](void){
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
        uint64_t timestamp_ms = duration.count();
        std::cout << "timer 1, now time :" << timestamp_ms << std::endl;
    });

    timer.addTimer(2000, [&](void){
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
        uint64_t timestamp_ms = duration.count();
        std::cout << "timer 2, now time :" << timestamp_ms << std::endl;
    });

    std::thread t([&](){
        timer.run();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    timer.stop();
    t.join();

    return 0;
}
