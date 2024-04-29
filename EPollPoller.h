#pragma once
#include "Poller.h"//������Ҫ��ϸ���������������ǰ������ ���Ҫ����ͷ�ļ�
#include <vector>
#include <sys/epoll.h>
#include "Timestamp.h"
/*
��������ļ��������¼��Ƿ񴥷��Լ����ط����¼����ļ��������Լ������¼���ģ�����Poller������һ��Poller�����Ӧһ���¼�������
��multi-reactorģ���У��ж���reactor���ж���Poller��
epoll��ʹ��
epoll_create ����epollfd_
epoll_ctl add/mod/del
epoll_wait
*/

class Channel;
class EPollPoller:public Poller
{
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;//override�ñ�������鸲�� ��Ϊ����������һ���麯������
    //��д����Poller�ĳ��󷽷�
    Timestamp poll(int timeoutMs,ChannelList *activateChannels) override;//epoll_wait
    void updateChannel(Channel *channel) override;//epoll_ctl
    void removeChannel(Channel *channel) override;//epoll_ctl

private:
    static const int kInitEventListSize=16; //��vector��ʼ�ĳ���
    //��д��Ծ������
    void fillActivateChannels(int numEvents,ChannelList *activateChannels) const;//��д��Ծchannel
    //����channelͨ�� ������epoll_ctl
    void update(int operation,Channel* channel);
    using EventList=std::vector<epoll_event>;//epoll_event��һ���ṹ��
    int epollfd_;//������� epoll_create����
    EventList events_;//��Ϊepoll_wait�ĵڶ��������������¼���������
};