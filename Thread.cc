#include "Thread.h"
#include "CurrentThread.h"
#include <semaphore.h>

std::atomic_int Thread::numCreated_(0);//静态成员变量需要在类外单独进行初始化 使用标准指定构造函数
Thread::Thread(ThreadFunc func,const std::string &name)
    :started_(false)
    ,joined_(false)
    ,tid_(0)
    ,func_(std::move(func))//将func底层资源给成员变量
    ,name_(name)
{
    setDefaultName();
}
Thread::~Thread()
{
    if (started_&&!joined_) //detach相当于设置成分离线程，变成守护线程 主线程结束后守护线程自动结束 不会出现孤儿线程 但join和detach不能同时执行 因为join是普通的工作线程
    {
        thread_->detach();//thread类提供的设置分离线程的方法
    }
}
void Thread::start()//一个Thread对象，记录的就是一个新线程的详细信息
{
    started_=true;
    sem_t sem;//信号量资源
    sem_init(&sem,false,0);
    //开启线程
    thread_=std::shared_ptr<std::thread>(new std::thread([&](){ //lambda表达式 产生线程对象并传线程函数 以引用方式&接收外部thread对象 可以访问成员变量
        //获取线程的tid值 也是子线程执行的第一行
        tid_=CurrentThread::tid();
        //信号量资源+1
        sem_post(&sem);
        //开启一个新线程，专门执行该线程函数 执行的是一个回调EventLoopThread::threadFunc
        func_();
    }));

    //这里必须等待获取上面新创建的新创建的线程的tid值
    sem_wait(&sem);//如果没有信号会将start阻塞住
    
}
void Thread::join()
{
    joined_=true;
    thread_->join();
}

void Thread::setDefaultName()
{
    int num=++numCreated_;
    if (name_.empty())
    {
        char buf[32]={0};
        snprintf(buf,sizeof buf,"Thread%d",num);
        name_=buf;
    }

}