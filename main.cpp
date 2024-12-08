
#include "minheap.h"
#include "rbtree.h"
#include "zkiplist.h"

#include <cstdint>
#include <iostream>
#include <functional>
#include <chrono>
#include <ctime>
#include <thread>

enum TimeType {
    MIN_HEAP,
    RBTREE,
    ZKIPLIST,
};

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

struct TimeNodeRb {
    rbtree_node env;
    CallBack cb;
};

struct TimeNodeZp {
    zskiplistNode env; 
    CallBack cb;   
};

class Timer {
public:
    Timer(uint32_t size);
    Timer(uint32_t size, TimeType Type);
    Timer() = delete;
    ~Timer();

    int addTimer(uint64_t time, CallBack cb);

    void run();

    void stop();

private:
    uint64_t GetNowTime();

    void AddMinHeapTimer(uint64_t time, CallBack cb);

    void AddRbtreeTimer(uint64_t time, CallBack cb);

    void AddZskiplistTimer(uint64_t time, CallBack cb);
private:
    min_heap* _heap;
    rbtree*   _rbtree;
    bool _close;
    TimeType _Type;
    zskiplist* _zskiplist;
};

Timer::Timer(uint32_t size) {
    min_heap_init(&_heap, size);
    _close = false;
}

Timer::Timer(uint32_t size, TimeType Type) {
    switch (Type) {
    case  MIN_HEAP:
        min_heap_init(&_heap, size);
        break;
    case RBTREE:
        _rbtree = new rbtree();
        if(_rbtree) {
            _rbtree->sentinel = new rbtree_node();
            rbtree_init(_rbtree, _rbtree->sentinel);
        }
        break;
    case ZKIPLIST:
        _zskiplist = zslCreate();
    }
    _close = false;
    _Type = Type;
}

Timer::~Timer() {
    switch (_Type) {
        case MIN_HEAP:
            min_heap_free(_heap);
        break;
        case RBTREE:
            delete _rbtree->sentinel;
            delete _rbtree;      
        break;
        case ZKIPLIST:
            zslFree(_zskiplist);
        break;
    }
}

int Timer::addTimer(uint64_t time, CallBack cb) {
    switch (_Type) {
    case MIN_HEAP:
        AddMinHeapTimer(time, cb);
        break;
    case RBTREE:
        AddRbtreeTimer(time, cb);
        break;
        case ZKIPLIST:
        AddZskiplistTimer(time, cb);
        break;
    }
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
        uint64_t sleep = 50;
        uint64_t now = GetNowTime();
        switch (_Type) {
            case MIN_HEAP: {
                timer_entry *entry = nullptr;
                if (!min_heap_top(_heap, &entry)) {
                    if (entry->time <= now) {
                        if (!min_heap_pop(_heap, &entry)) {
                            TimeNode *node = container_of(entry, TimeNode, env);
                            if (node) {
                                node->cb();
                            }

                            delete node;
                        }
                    } else {
                        sleep = entry->time - now;
                    }
                }
            }
            case RBTREE: {
                rbtree_node* node = rbtree_min(_rbtree);
                if(!node) {
                    break;
                }

                if(node->key <= now) {
                    rbtree_delete(_rbtree, node);
                    TimeNodeRb* n = container_of(node, TimeNodeRb, env);
                    n->cb();

                    delete n;
                } else {
                    sleep = node->key - now; 
                }
            }
            case ZKIPLIST: {
                zskiplistNode* node = zslMin(_zskiplist);
                if(!node) {
                    break;
                }

                if(node->score <= now) {
                    zslDeleteHead(_zskiplist);
                    TimeNodeZp* n = container_of(node, TimeNodeZp, env);
                    if(n) {
                        n->cb();
                        zslDeleteNode(&n->env);
                        delete n;
                    }
                }else {
                    sleep = node->score - now; 
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(sleep));
    }
}


void Timer::stop() {
    _close = true;
}

void Timer::AddMinHeapTimer(uint64_t time, CallBack cb) {
    TimeNode* node = new TimeNode();
    node->env.time = GetNowTime() + time;
    node->cb = cb;
    min_heap_push(_heap, &node->env);
}

void Timer::AddRbtreeTimer(uint64_t time, CallBack cb) {
    TimeNodeRb* node = new TimeNodeRb();
    node->env.key = GetNowTime() + time;
    node->cb = cb;
    rbtree_insert(_rbtree, &node->env);
}

void Timer::AddZskiplistTimer(uint64_t time, CallBack cb) {
    TimeNodeZp* node = new TimeNodeZp();
    node->env.score = GetNowTime() + time;
    node->env.level = (zskiplistLevel*)zslCreateLevel(zslRandomLevel());
    node->cb = cb;
    zslInsert(_zskiplist, &node->env);
}

int main(int, char**){

    Timer timer(0, ZKIPLIST);

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
