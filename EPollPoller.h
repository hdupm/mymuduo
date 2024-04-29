#pragma once
#include "Poller.h"//基础需要详细基类情况不仅仅是前置声明 因此要包含头文件
#include <vector>
#include <sys/epoll.h>
#include "Timestamp.h"
/*
负责监听文件描述符事件是否触发以及返回发生事件的文件描述符以及具体事件的模块就是Poller。所以一个Poller对象对应一个事件监听器
在multi-reactor模型中，有多少reactor就有多少Poller。
epoll的使用
epoll_create 创造epollfd_
epoll_ctl add/mod/del
epoll_wait
*/

class Channel;
class EPollPoller:public Poller
{
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;//override让编译器检查覆盖 因为基类中这是一个虚函数方法
    //重写基类Poller的抽象方法
    Timestamp poll(int timeoutMs,ChannelList *activateChannels) override;//epoll_wait
    void updateChannel(Channel *channel) override;//epoll_ctl
    void removeChannel(Channel *channel) override;//epoll_ctl

private:
    static const int kInitEventListSize=16; //给vector初始的长度
    //填写活跃的连接
    void fillActivateChannels(int numEvents,ChannelList *activateChannels) const;//填写活跃channel
    //更新channel通道 本质是epoll_ctl
    void update(int operation,Channel* channel);
    using EventList=std::vector<epoll_event>;//epoll_event是一个结构体
    int epollfd_;//红黑树根 epoll_create返回
    EventList events_;//作为epoll_wait的第二个参数，返回事件发生数量
};