#pragma once
#include <functional>
#include "noncopyable.h"
#include <vector>
#include <atomic>
#include "Timestamp.h"
#include <memory>
#include <mutex>
#include "CurrentThread.h"

class Channel;
class Poller;


//事件循环类 主要包含了两个大模块Channel Poller(epoll的抽象)
class EventLoop:noncopyable
{
public:
    using Functor=std::function<void()>;//类型重命名
    EventLoop();
    ~EventLoop();

    //开启事件循环
    void loop();
    //退出事件循环
    void quit();
    Timestamp pollReturnTime() const{return pollReturnTime_;};
    //在当前loop中执行cb
    void runInLoop(Functor cb);
    //把cb放入队列中，唤醒loop所在的线程，执行cb
    void queueInLoop(Functor cb);//在队列中转到loop所在线程再执行
    //用来唤醒loop所在的线程的
    void wakeup();
    //EventLoop的方法=》 Poller的方法
    void updateChannel(Channel *Channel);//调用poller更改channel事件
    void removeChannel(Channel *Channel);
    //判断EventLoop对象是否在自己的线程里面
    bool hasChannel(Channel *channel);

    bool isInLoopThread()const {return threadId_==CurrentThread::tid();};//eventloop是否在所创建的线程内 比较id
private:
    void handleRead();//唤醒线程用 wake up
    void doPendingFunctors();//执行回调    

    using ChannelList=std::vector<Channel*>;
    std::atomic_bool looping_;//转成原子操作(进行某一操作时不被打断)的布尔值 底层通过CAS实现的 这两个和loop是否循环相关用来控制
    std::atomic_bool quit_;//一般是其他线程调用eventloop的quit 标识退出loop循环
    
    const pid_t threadId_;//记录当前loop所在线程的id

    Timestamp pollReturnTime_;//poller返回发生事件的channels(activatechannels)的时间点
    std::unique_ptr<Poller> poller_;

    int wakeupFd_;//主要作用，当mainLoop获取一个新用户的channel，通过轮询算法选择一个subloop(即subreactor)，通过该成员唤醒subloop处理channel
    std::unique_ptr<Channel> wakeupChannel_;//里面包含感兴趣事件和wakefd

    ChannelList activateChannels_;
    //Channel *currentActivateChannel_;

    std::atomic_bool callingPendingFunctors_;//标识当前loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_;//存储loop需要执行的所有的回调操作 保存的是其他线程希望你这个EventLoop线程执行的函数
    std::mutex mutex_;//互斥锁来保护上面vector容器的线程安全操作

};