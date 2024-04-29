#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"
#include <memory>

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop,const std::string &nameArg)
    :baseLoop_(baseLoop)
    ,name_(nameArg)
    ,started_(false)
    ,numThreads_(0)
    ,next_(0)//轮询下标
{}

EventLoopThreadPool::~EventLoopThreadPool()//析构不用做什么 注意不需要关注vector中指针即外部资源需要delete 因为线程绑定的loop都是栈上的对象
{}

void EventLoopThreadPool::start(const ThreadInitCallback &cb)//开启事件循环线程 接收上级TcpServer给出的回调函数并初始化线程
{
    started_=true;
    for (int i=0;i<numThreads_;++i)//按照线程数目创建线程
    {
        char buf[name_.size()+32];//线程池+下标序号作为底层线程名字
        snprintf(buf,sizeof buf,"%s%d",name_.c_str(),i);
        EventLoopThread *t=new EventLoopThread(cb,buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->startLoop());//上面只是创建 现在是底层创建线程，绑定一个新的EventLoop，并返回该loop的地址
    }
    //如果线程池数目为0说明整个服务端只有一个主线程，运行着baseloop
    if (numThreads_==0&&cb)
    {
        cb(baseLoop_);//执行用户设置的回调
    }
}
//如果工作在多线程中，baseloop_默认以轮询方式分配channel给subloop
EventLoop* EventLoopThreadPool::getNextLoop()//mainloop(baseloop) 通过轮询方式从子线程取loop
{
    EventLoop *loop=baseLoop_;//指向mainloop 如果没有底层调用那么loop就是baseloop
    if (!loops_.empty())//通过轮询获取下一个处理事件的loop
    {
        loop=loops_[next_];//next_初始化为0 将next_对应的线程赋值给loop
        ++next_;
        if (next_>=loops_.size())
        {
            next_=0;
        }
    }
    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()//池里所有loops
{
    if (loops_.empty())//说明只有baseloop
    {
        return std::vector<EventLoop*>(1,baseLoop_);
    }
    else
    {
        loops_;///////////////////////
    }
}