//绑定loop和thread 让loop运行在thread 起一线程创建一个eventloop
#pragma once
#include "noncopyable.h"
#include <functional>
#include "Thread.h"
#include <mutex>
#include <condition_variable>//条件变量
#include <string>

class EventLoop;

class EventLoopThread:noncopyable
{
public:
    using ThreadInitCallback=std::function<void(EventLoop*)>;//创建eventloop后进行初始化操作

    EventLoopThread(const ThreadInitCallback &cb,const std::string &name= std::string());//给定值不能多个 声明时候给定义就不用给了
    ~EventLoopThread();
    EventLoop* startLoop();//开启循环

private:
    void threadFunc();//线程函数 创建loop
    EventLoop *loop_;
    bool exiting_;//是否退出循环
    Thread  thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;
};