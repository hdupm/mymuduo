#include "Channel.h"
#include <sys/epoll.h>
#include "Logger.h"
#include "EventLoop.h"

const int Channel::kNoneEvent=0;
const int Channel::kReadEvent=EPOLLIN|EPOLLPRI;
const int Channel::kWriteEvent=EPOLLOUT;

//EventLoop:ChannelList Poller
Channel::Channel(EventLoop *loop,int fd)
    :loop_(loop),fd_(fd),events_(0),revents_(0),index_(-1),tied_(false) //index_初始化-1表示未添加到poller中
{
}
Channel::~Channel()
{    
}

//channel的tie方法什么时候调用? 一个TcpConnection新连接创建的时候 TcpConnection=> Channel 强智能指针记录的对象不会被释放
//channel有一个弱智能指针绑定上层对象 TcpConnection
//因为Channel的回调函数绑定的对象都是TcpConnection 为了防止Channel调用回调函数时TcpConnection被移除 发生未知错误
void Channel::tie(const std::shared_ptr<void> &obj)//强转智能指针
{
    tie_=obj;//绑定观察对象
    tied_=true;//已经绑定过了
}

/*
当改变channel所表示fd的events事件后，update负责在poller里面更改fd相应的事件epoll_ctl
EventLoop=>ChannelList Poller 二者组成 二者通过EventLoop进行交互
update()和remove()调用的实际是poll的方法 因为channel无法直接访问poll
*/
void Channel::update()
{
    //通过channel所属的EventLoop，调用poller的相应方法，注册fd的events事件
    //add code...
    loop_->updateChannel(this);//将channel传进去

}

//在channel所属的EventLoop中，把当前的channel删除掉
//eventloop的容器里面包含了所有channel，vector<Channel*>
void Channel::remove()
{   
    //add code...
    loop_->removeChannel(this);
}

//处理事件
void Channel::handleEvent(Timestamp receiveTime)
{   
    //如果监听过当前channel
    if (tied_)
    {
        std::shared_ptr<void> guard=tie_.lock();//从弱智能指针提升为强智能指针 用于确保 Channel 对象仍然有效（即仍然存在），并在需要时延长其生命周期，以避免在处理事件期间出现悬挂指针（dangling pointer）的情况
        if (guard) //如果guard不为空 说明当前channel仍存活 存活则继续处理
        {
            handleEventWithGuard(receiveTime);
        }
        
    }
    else//提升失败则不调用
    {
        handleEventWithGuard(receiveTime);
    }
}

//根据poller通知的channel发生的具体事件，由channel负责调用具体的回调操作
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("channel handleEvent revents:%d\n",revents_);
    if ((revents_& EPOLLHUP)&&!(revents_& EPOLLIN))
    {
        if (closeCallback_)//回调不为空
        {
            closeCallback_();
        }
    }
    if (revents_ &(EPOLLERR))//发生错误事件
    {
        if (errorCallback_)
        {
            errorCallback_();
        }
    }
    if (revents_ &(EPOLLIN|EPOLLPRI))
    {
        if (readCallback_)
        {
            readCallback_(receiveTime);
        }
    }
    if (revents_ &(EPOLLOUT))
    {
        if (writeCallback_)
        {
            writeCallback_();
        }
    }
}