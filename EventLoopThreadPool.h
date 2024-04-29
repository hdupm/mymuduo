//线程池 底层管理eventloopthread
#pragma once 
#include "noncopyable.h"
#include <functional>
#include <string>
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool:noncopyable
{
public:
    using ThreadInitCallback=std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop *baseLoop,const std::string &nameArg);
    ~EventLoopThreadPool();//析构不用做什么

    void setThreadNum(int numThreads){numThreads_=numThreads;}
    void start(const ThreadInitCallback &cb=ThreadInitCallback());//开启事件循环线程
    //如果工作在多线程中，baseloop_默认以轮询方式分配channel给subloop
    EventLoop* getNextLoop();//mainloop(baseloop)
    std::vector<EventLoop*> getAllLoops();//池里所有loops
    bool started() const{return started_;}
    const std::string name() const {return name_;}

private:
    EventLoop *baseLoop_;//最开始用户创建的loop mainloop
    std::string name_;
    bool started_;
    int numThreads_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;//包含所有创建事件循环的线程
    std::vector<EventLoop*> loops_;//事件线程eventloop的指针


};