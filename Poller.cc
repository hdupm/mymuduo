#include "Poller.h"
#include "Channel.h"//需要用到功能而不是声明 因此要用头文件

/*
cc里面没有newDefaultPoller的具体实现是因为具体实现需要调用pollpoller.h或者epollpoller.h，而基类的文件中不应该调用派生类的头文件
因此在其他文件中进行具体实现 详细实现在defaultpoller文件中
*/

Poller::Poller(EventLoop *loop)
    :ownerLoop_(loop)//构造函数负责将poller所属的eventloop记录
{
}

bool Poller::hasChannel(Channel *channel) const
{
    auto it=channels_.find(channel->fd());
    return it!=channels_.end()&&it->second==channel;//表示sockfd是否存在且为当前channel
}