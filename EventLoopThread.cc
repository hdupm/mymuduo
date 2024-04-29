#include "EventLoopThread.h"
#include "EventLoop.h"


EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
                                 const std::string &name)
    :loop_(nullptr)
    ,exiting_(false)
    ,thread_(std::bind(&EventLoopThread::threadFunc,this),name)//绑定回调函数
    ,mutex_()
    ,cond_()
    ,callback_(cb)
{

}

EventLoopThread::~EventLoopThread()
{
    exiting_=true;
    if (loop_!=nullptr)
    {
        loop_->quit();//线程退出
        thread_.join();
    }
}
EventLoop* EventLoopThread::startLoop()//开启循环 得到新线程loop指针
{
    thread_.start();//启动底层的新线程 执行下发给底层线程的回调函数threadFunc
    EventLoop *loop=nullptr;//需要等start执行下面回调函数完毕 loop指针指向对象才能继续执行
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while(loop_==nullptr)//说明子线程还未执行完
        {
            cond_.wait(lock);//等待互斥锁
        }
        loop=loop_;
    }
    return loop;//返回新线程里面单独运行的loop对象

}

//下面这个方法，是在单独的新线程里面运行的
void EventLoopThread::threadFunc()//线程函数 创建loop
{
    EventLoop loop;//创建一个独立的event loop，和上面的线程是一一对应的 one loop per thread 真正执行loop的线程
    if (callback_)//如果有回调函数 对应Tcpserver中ThreadInitCallback函数 可以对loop进行初始化操作
    {
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_=&loop;//该函数成员对象
        cond_.notify_one();//条件变量通知一次
    }
    
    loop.loop();//EventLoop loop函数=>开启Poller.poll loop会一直循环工作
    //进入到下面程序说明服务器要关闭了
    std::unique_lock<std::mutex> lock(mutex_);//释放锁
    loop_=nullptr;

}