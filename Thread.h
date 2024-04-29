//thread类只关注于一个线程
#pragma once
#include "noncopyable.h"
#include <functional>
#include <thread>//用c++的线程类 c的线程类较繁琐
#include <memory>
#include <unistd.h>
#include <atomic>
#include <string>

class Thread:noncopyable
{
public:
    using ThreadFunc=std::function<void()>;//如果有函数参数 那么可以用绑定器和函数对象

    explicit Thread(ThreadFunc,const std::string& name=std::string());
    ~Thread();
    void start();
    void join();
   
    bool started() const{return started_;}
    pid_t tid() const {return tid_;}//不是真正的线程号
    const std::string& name() const {return name_;}

    static int numCreated() {return numCreated_;}
private:
    void setDefaultName();
    bool started_;
    bool joined_;
    //这里thread和linux中pthread一样，api调用后线程直接开始启动，但我们需要控制线程对象启动的时间 需要智能指针封装
    std::shared_ptr<std::thread>thread_;
    pid_t tid_;
    ThreadFunc func_;//存储线程函数
    std::string name_;//线程名字
    static std::atomic_int numCreated_;
};