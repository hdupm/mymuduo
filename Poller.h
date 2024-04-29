#pragma once
#include "noncopyable.h"
#include <vector>
#include <unordered_map>
#include "Timestamp.h"

class Channel;//指针大小和具体类型无关 具体类型之和指针的偏移有关
class EventLoop;

//muduo库中多路事件分发器的核心IO复用模块 主要用来事件监听
//虚函数提供接口作用
class Poller : noncopyable
{
public:
    using ChannelList=std::vector<Channel*>;//起别名
    Poller(EventLoop* loop);
    virtual ~Poller()=default;
    //给所有IO复用保留统一的接口
    virtual Timestamp poll(int timeoutMs,ChannelList *activateChannels)=0;//当前正在运行的channel 启用epoll_wait
    virtual void updateChannel(Channel *channel)=0;//接收的都是channel的指针 channel中update()返回的也是channel指针 启用epoll_ctl
    virtual void removeChannel(Channel *channel)=0;

    //判断参数channel是否在当前Poller中
    bool hasChannel(Channel *channel) const;//判断是否包含某个channel
    //EventLoop可以通过该接口获取默认的IO复用的具体实现
    static Poller* newDefaultPoller(EventLoop *loop);//获取事件循环默认的poller

protected:
    //map的key：sockfd value:sockfd所属的channel通道类型
    //通过发生事件的fd可以找到对应的channel，里面记录了回调函数
    using ChannelMap=std::unordered_map<int,Channel*>;
    ChannelMap channels_;//负责记录 文件描述符 —> Channel的映射，也帮忙保管所有 注册在这个Poller上的Channel
private:
    EventLoop *ownerLoop_;//定义poller所属的事件循环EventLoop对象
};