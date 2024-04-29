#include "EventLoop.h"
#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include "Logger.h"
#include <errno.h>
#include "Poller.h"
#include "Channel.h"

//防止一个线程创建多个EventLoop 当对象被创建出来后 就指向对象 __thread是thread_local的机制 具有线程周期 如果不加就是全局变量，所有线程共享。这样每个线程都有这个副本
__thread EventLoop *t_loopInThisThread=nullptr;

//定义默认的Poller IO复用接口的超时时间 10s 也就是epoll_wait每10秒轮询一次并返回结果 像日志打印也是10s设置的
const int kPollTimeMs=10000;

//创建wakeupfd，用来notify唤醒subReactor处理新来的channel(采用轮询方式)
int createEventfd()//调用系统接口
{
    int evtfd=::eventfd(0,EFD_NONBLOCK|EFD_CLOEXEC);//创建wakeup fd，用来通知睡觉的eventloop有新连接用户的channel需要处理
    if (evtfd<0)
    {
        LOG_FATAL("eventfd error:%d",errno);
    }
    return evtfd;    
}

EventLoop::EventLoop()
    :looping_(false)
    ,quit_(false)
    ,callingPendingFunctors_(false)
    ,threadId_(CurrentThread::tid())
    ,poller_(Poller::newDefaultPoller(this))//获取默认poller
    ,wakeupFd_(createEventfd())
    ,wakeupChannel_(new Channel(this,wakeupFd_)) //每个subreactor都监听了wakeupfd 而mainreactor notify wakeupfd，相应的loop就被唤醒
    //,currentActivateChannel_(nullptr)
{
    LOG_DEBUG("EventLoop created %p in thread %d \n",this,threadId_);
    if (t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n",t_loopInThisThread,threadId_);
    }
    else
    {
        t_loopInThisThread=this;
    }

    //设置wakeupfd的事件类型以及发生时间后的回调操作(唤醒subreactor/eventloop) 如果一直没有事情，poll不会返回。但是这时候有事情做(处理新的连接)，必须让poll返回
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead,this)); //绑定器和函数对象
    //每一个eventloop都将监听wakeupchannel的EPOLLIN读事件了
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()//资源回收
{
    wakeupChannel_->disableAll();//对所有事件感兴趣
    wakeupChannel_->remove();//从poller中移除
    ::close(wakeupFd_);
    t_loopInThisThread=nullptr;
}

//开启事件循环(驱动底层poller来执行poll方法)
void EventLoop::loop()
{
    looping_=true;//开启循环
    quit_=false;//未退出

    LOG_INFO("EventLoop %p start looping\n",this);
    
    while(!quit_)
    {
        activateChannels_.clear();
        //监听两类fd 一种是client的fd(客户端通信)，一种wakeupfd(mainloop唤醒subloop)
        pollReturnTime_=poller_->poll(kPollTimeMs,&activateChannels_);//传入超时时间
        for (Channel *channel:activateChannels_)//得到发生事件的channel动态数组activateChannels_
        {   
            //poller监听哪些channel发生事件了，然后上报给EventLoop，通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        //执行当前EventLoop事件循环需要处理的回调操作
        /*
        IO线程 mainLoop accept工作 返回fd《=打包成channel 唤醒并分发给subloop
        mainloop事先注册一个回调cb (需要subloop来执行) wakeup subloop后，执行下面的方法，执行之前mainloop注册的cb操作
        比如接收新的channel        
        */
        doPendingFunctors();//执行回调
        
    }
    LOG_INFO("EventLoop %p stop looping.\n",this);
    looping_=false;
}

//退出事件循环有两种情况 1.loop在自己的线程中调用quit 2.在非loop的线程中调用loop的quit
void EventLoop::quit()
{
    quit_=true;
    //!isInLoopThread()说明不在自己线程中quit 另外的线程在quit_置true后自动跳出上面的while循环
    //如果是在其他线程中调用的quit，比如在一个subloop(worker)中，调用了mainLoop(IO)的quit
    /*
                        mainLoop (每一个loop都有一个wakeupfd能唤醒对应loop)
        =============================================生产者-消费者的线程安全队列 muduo库没有 main和sub通过wakeupfd直接通信

    subLoop1        subLoop2            subLoop3
    
    */
    if (!isInLoopThread())
    {
        wakeup();//需要唤醒主线程mainloop
    }
}

//在当前loop中执行cb
void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread())//在当前的loop线程中执行cb(cb都和channel有关)
    {
        cb();
    }
    else//在非当前loop线程中执行cb，就需要Loop所在线程，执行cb
    {
        queueInLoop(cb);
    }
}

//把cb放入队列中，唤醒loop所在的线程，执行cb(也说明cb不在当前loop中)
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);//智能锁
        pendingFunctors_.emplace_back(cb);//vector底层末尾直接构造cb，而不是拷贝构造
    }

    //loop不是此线程的，需要唤醒相应的，需要执行的上面回调操作的loop的线程了
    //callpendingfunctors_的意思是：当前loop正在执行回调，但是loop有了新的回调 为了防止阻塞需要wakeup让线程继续执行任务
    if (!isInLoopThread()||callingPendingFunctors_)
    {
        wakeup();//唤醒loop所在线程
    }
}

//用来唤醒loop所在的线程的 向wakeupfd_写一个数据 这样wakeupchannel响应发生读事件，当前loop线程就会被唤醒
void EventLoop::wakeup()
{
    uint64_t one=1;
    ssize_t n=write(wakeupFd_,&one,sizeof one) ;
    if (n!=sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n",n);
    }

}

//EventLoop的方法=》 Poller的方法
void EventLoop::updateChannel(Channel *channel)//调用poller的方法更改channel事件
{
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}
//判断EventLoop对象是否在自己的线程里面
bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors()//唤醒后执行回调
{
    std::vector<Functor> functors;
    callingPendingFunctors_=true;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);//释放pendingFunctors_，避免mainloop阻塞等待 可以实现执行回调和mainloop向当前loop写回调的同时进行
    }

    for (const Functor &functor:functors)
    {
        functor();//执行当前loop需要执行的回调操作
    }
    callingPendingFunctors_ = false;    
}    

void EventLoop::handleRead() //用来读取wakeupfd的读事件
{
  uint64_t one = 1;//buf 8字节
  ssize_t n = read(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
    LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8",n) ;
  }
}