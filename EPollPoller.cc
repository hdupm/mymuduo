#include "EPollPoller.h"
#include "Logger.h"
#include <errno.h>
#include <unistd.h>
#include "Channel.h"
#include <string.h>

//��ʾchannel��δ��ӵ�poller�� channel�Ĳ���index_��ʼ��Ϊ-1
const int kNew=-1;
//��ʾchannel����ӵ�poller��
const int kAdded=1;
//channel��poller��ɾ��
const int kDeleted=2;

EPollPoller::EPollPoller(EventLoop *loop) //������ʾ��ǰchannel��epoll��״̬
    :Poller(loop)//���ȵ��û��๹�캯������ʼ���ӻ���̳еĳ�Ա   ::����ָ��ȫ�������ռ�
    ,epollfd_(::epoll_create1(EPOLL_CLOEXEC)) //EPOLL_CLOEXEC��flag
    ,events_(kInitEventListSize) //vector<epoll_event> ��vector�������Դ洢
{
    if (epollfd_<0)
    {
        LOG_FATAL("epoll_create error:%d\n",errno);
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

//����poller
/*
���ⲿ����poll������ʱ�򣬸÷����ײ���ʵ��ͨ��epoll_wait��ȡ����¼��������Ϸ����¼���fd�����Ӧ�������¼�������֪��ÿ��fd������һ��Channel��װ�ģ�ͨ����ϣ��channels_���Ը���fd�ҵ���װ���fd��Channel��
���¼���������������fd�������¼�д�����Channel�е�revents��Ա�����С�Ȼ������Channelװ��activeChannels�У�����һ��vector<Channel*>��������������������poll֮������õ��¼��������ļ��������activeChannels_��
*/
Timestamp EPollPoller::poll(int timeoutMs,ChannelList *activateChannels)//epoll_wait �����ǳ�ʱʱ�� �Դ����list���м�����Щchannel�����¼� timeoutMs������ʱ�� ����һ��ʱ�����뷵�� ��Ϊ�Ƿ�����
{
    //ʵ����Ӧ����LOG_DEBUG�����־��Ϊ����
    LOG_INFO("func=%s => fd total count:%lu \n",__FUNCTION__,channels_.size());

    ///static_cast<int>������ǿת ��unsignedתΪsigned����
    int numEvents=::epoll_wait(epollfd_,&*events_.begin(),static_cast<int> (events_.size()),timeoutMs);//�ڶ������������Ǵ�ŷ���event�¼������� Ϊ�˷���������vector events_begin()��������Ԫ�ص�ַ ����������Ԫ�ص�ֵ
    int saveErrno=errno;//��������þֲ��������
    Timestamp now(Timestamp::now());

    if (numEvents>0)//�����¼�����
    {
        LOG_INFO("%d events happened \n",numEvents);
        fillActivateChannels(numEvents,activateChannels);
        if (numEvents==events_.size())//��Ҫ���� events_�Ƕ����vector����
        {
            events_.resize(events_.size()*2);
        }
    }
    else if (numEvents==0)//���ڳ�ʱ
    {
        LOG_DEBUG("%s timeout!\n",__FUNCTION__);
    }
    else{
        //�ⲿ�ж�
        if (saveErrno!=EINTR)
        {
            errno=saveErrno;//��ֹ���������ͻ
            LOG_ERROR("EPollPoller::poll() err!");
        }
    }
    return now;//���ؾ���ʱ���
}

//channel update remove =>EventLoop updateChannel removeChannel=>Poller
/*
                    EventLoop=> poller.poll
        ChannelList             Poller
    �������channel(fd)  >=    ChannelMap <fd,channel*> ��pollerע����
    �����Ƿ���pollerע���
*/
void EPollPoller::updateChannel(Channel *channel) //epoll_ctl
{
    const int index=channel->index();
    LOG_INFO("func=%s => fd=%d events=%d index=%d \n",__FUNCTION__,channel->fd(),channel->events(),index);//�������Ǳ�����Ĭ���ṩ��
    if (index==kNew||index==kDeleted)
    {
        if (index==kNew)//˵��û��poller��ӹ�
        {
            int fd=channel->fd();
            channels_[fd]=channel;
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD,channel);
    }
    else //channel�Ѿ���poller��ע�����
    {
        int fd=channel->fd();
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL,channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD,channel);
        }
    }

}

//��poller��ɾ��channel
void EPollPoller::removeChannel(Channel *channel) //epoll_ctl ������channellist�У���Ҫ��epoll��channelmap��remove��
{
    int fd=channel->fd();    
    channels_.erase(fd);
    LOG_INFO("func=%s => fd=%d\n",__FUNCTION__,fd);
    int index=channel->index();
    if (index==kAdded)
    {
        update(EPOLL_CTL_DEL,channel);
    }
    channel->set_index(kNew);

}

void EPollPoller::fillActivateChannels(int numEvents,ChannelList *activateChannels) const//��д��Ծchannel
{
    for (int i=0;i<numEvents;i++)
    {
        Channel *channel=static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activateChannels->push_back(channel);//eventloop���õ�������poller�������ص����з���ʱ���channel�б���
    }
}

//����channelͨ�� epoll_ctl add/mod/del    
void EPollPoller::update(int operation,Channel* channel)
{
    epoll_event event;
    bzero(&event,sizeof event);//���Ϊ0 ������memsetҲ���ԣ�����Ҫ���ڶ���������Ϊ��������
    
    int fd=channel->fd();

    event.events=channel->events();//����fd����Ȥ�¼�
    event.data.fd=fd;
    event.data.ptr=channel;//data��epoll_data_t���ͣ�void *ptr�����fd��������ptrָ��fd��Ӧ��channel ������Я��fd����
    
    if (::epoll_ctl(epollfd_,operation,fd,&event)<0)
    {
        if (operation==EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_stl del error:%d\n",errno);//�����ɾ��ʧ�ܣ����鲻������ ��ӡ������Ϣ����
        }
        else
        {
            LOG_FATAL("epoll_stl add/mod error:%d\n",errno);//�޸�/����ʧ�ܵ���FATAL �Զ�exit
        }
    }
}