#include "Poller.h"
#include <stdlib.h>//��ȡ��������
#include "EPollPoller.h"

Poller* Poller::newDefaultPoller(EventLoop *loop)
{
    if (::getenv("MUDUO_USE_POLL"))//��ȡ��������
    {
        return nullptr;//����poller��ʵ��
    }
    else
    {
        return new EPollPoller(loop);//����epoller��ʵ��
    }
}