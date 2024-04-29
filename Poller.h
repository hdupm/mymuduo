#pragma once
#include "noncopyable.h"
#include <vector>
#include <unordered_map>
#include "Timestamp.h"

class Channel;//ָ���С�;��������޹� ��������֮��ָ���ƫ���й�
class EventLoop;

//muduo���ж�·�¼��ַ����ĺ���IO����ģ�� ��Ҫ�����¼�����
//�麯���ṩ�ӿ�����
class Poller : noncopyable
{
public:
    using ChannelList=std::vector<Channel*>;//�����
    Poller(EventLoop* loop);
    virtual ~Poller()=default;
    //������IO���ñ���ͳһ�Ľӿ�
    virtual Timestamp poll(int timeoutMs,ChannelList *activateChannels)=0;//��ǰ�������е�channel ����epoll_wait
    virtual void updateChannel(Channel *channel)=0;//���յĶ���channel��ָ�� channel��update()���ص�Ҳ��channelָ�� ����epoll_ctl
    virtual void removeChannel(Channel *channel)=0;

    //�жϲ���channel�Ƿ��ڵ�ǰPoller��
    bool hasChannel(Channel *channel) const;//�ж��Ƿ����ĳ��channel
    //EventLoop����ͨ���ýӿڻ�ȡĬ�ϵ�IO���õľ���ʵ��
    static Poller* newDefaultPoller(EventLoop *loop);//��ȡ�¼�ѭ��Ĭ�ϵ�poller

protected:
    //map��key��sockfd value:sockfd������channelͨ������
    //ͨ�������¼���fd�����ҵ���Ӧ��channel�������¼�˻ص�����
    using ChannelMap=std::unordered_map<int,Channel*>;
    ChannelMap channels_;//�����¼ �ļ������� ��> Channel��ӳ�䣬Ҳ��æ�������� ע�������Poller�ϵ�Channel
private:
    EventLoop *ownerLoop_;//����poller�������¼�ѭ��EventLoop����
};