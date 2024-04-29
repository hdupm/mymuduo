#include "Poller.h"
#include <stdlib.h>//获取环境变量
#include "EPollPoller.h"

Poller* Poller::newDefaultPoller(EventLoop *loop)
{
    if (::getenv("MUDUO_USE_POLL"))//获取环境变量
    {
        return nullptr;//生成poller的实例
    }
    else
    {
        return new EPollPoller(loop);//生成epoller的实例
    }
}