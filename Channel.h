#pragma once
#include "noncopyable.h"
#include <functional>
#include "Timestamp.h"//头文件尽量少 尽量用前置声明
#include <memory>
/*
Channel理解为通道，封装了sockfd和其感兴趣的event，如EPOLLIN、EPOLLOUT事件
还绑定了poller返回的具体事件
理清楚EventLoop、Channel、Poller之间的关系 《=Reactor模型上对应Demultiplex
*/
class EventLoop;//类型前置声明 因为只用这个类型定义的指针所以前置声明即可

class Channel:noncopyable
{
public:
    using EventCallback=std::function<void()>; //事件回调   这里用c++中using代替typedef 定义回调函数EventCallback类型
    using ReadEventCallback=std::function<void(Timestamp)>; //只读事件回调
    Channel(EventLoop *Loop,int fd);
    ~Channel();

    //fd得到poller通知以后，处理事件的
    /*
    当调用epoll_wait()后，可以得知事件监听器上哪些Channel（文件描述符）发生了哪些事件，事件发生后自然就要调用这些Channel对应的处理函数。
    Channel::HandleEvent，让每个发生了事件的Channel调用自己保管的事件处理函数。每个Channel会根据自己文件描述符实际发生的事件（通过Channel中的revents_变量得知）和感兴趣的事件（通过Channel中的events_变量得知）
    来选择调用read_callback_和/或write_callback_和/或close_callback_和/或error_callback_。

    */
    void handleEvent(Timestamp receiveTime);

    //设置回调函数对象 Channel为这个文件描述符保存的各事件类型发生时的处理函数
    void setReadCallback(ReadEventCallback cb) { readCallback_=std::move(cb);} //调用函数对象赋值操作 这个形参对象不需要 将左值转成右值
    void setWriteCallback(EventCallback cb){writeCallback_=std::move(cb);}
    void setCloseCallback(EventCallback cb){closeCallback_=std::move(cb);}
    void setErrorCallback(EventCallback cb){ errorCallback_=std::move(cb);}

    //防止当channel被手动remove掉，channel还在指向回调操作
    void tie(const std::shared_ptr<void>&);

    int fd() const {return fd_;}//fd，Poller监听的对象 epoll_ctl 并不是fd想读就读 而是需要修改epoll
    int events()const{return events_;}
    //poll在使用监听事件 当事件监听器监听到一个fd发生了什么事件，通过set_revents()函数来设置revents值。通过这个函数可以将这个文件描述符实际发生的事件封装进这个Channel中。
    void set_revents(int revt){revents_=revt;}
    

    //设置fd相应的事件状态 代表着这个Channel为这个文件描述符保存的各事件类型发生时的处理函数
    /*
    外部通过这几个函数来告知Channel你所监管的文件描述符都对哪些事件类型感兴趣，并把这个文件描述符及其感兴趣事件注册到事件监听器（IO多路复用模块）上。
    这些函数里面都有一个update()私有成员方法，这个update其实本质上就是调用了epoll_ctl()。
    */
    void enableReading(){events_|=kReadEvent;update();}//将读事件相应位置置1 update()是通知epoll调用epoll_ctl将fd感兴趣事件添加到epoll里面
    void disableReading(){events_&=~kReadEvent;update();}//去掉读事件 将kReadEvent取反
    void enableWriting(){events_|=kWriteEvent;update();}
    void disableWriting(){events_&=~kWriteEvent;update();}
    void disableAll(){events_=kNoneEvent;update();}

    //返回fd当前的事件状态
    bool isNoneEvent() const{return events_==kNoneEvent;}//判断fd是否注册感兴趣事件
    bool isWriting() const{return events_& kWriteEvent;}
    bool isReading() const{return events_&kReadEvent;}
    int index(){return index_;}//和业务相关 和多路分发器本身逻辑关系不大
    void set_index(int idx){index_=idx;}

    //one loop per thread
    //channel属于一个eventloop 一个eventloop有一个poller 一个poller可以监听很多channel
    EventLoop* ownerLoop(){return loop_;}
    void remove();
private://放成员变量

    void update();

    void handleEventWithGuard(Timestamp receiveTime);//受保护的处理事件

    static const int kNoneEvent;//fd对没有事件感兴趣
    static const int kReadEvent;//fd对读事件感兴趣
    static const int kWriteEvent;//fd对写事件感兴趣

    EventLoop *loop_;//事件循环 此fd属于哪个EventLoop对象
    const int fd_;//fd是poller监听的对象 这个channel照看的文件描述符 需要向poller注册
    int events_;//注册fd感兴趣的事件集合
    int revents_;//poller返回的具体发生的事件集合 当事件监听器监听到一个fd发生了什么事件，通过Channel::set_revents()函数来设置revents值
    int index_;

    std::weak_ptr<void> tie_;//用弱智能指针监听对象 防止调用remove的channel 使用跨线程的对象生存状态的监听 void表示任何对象都可以接收
    bool tied_;//绑定当前对象

    //因为channel通道里面能够获知fd最终发生的具体的事件revents，所以他负责调用具体事件的回调操作
    ReadEventCallback readCallback_;//四个函数对象
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};