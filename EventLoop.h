#pragma once
#include <functional>
#include "noncopyable.h"
#include <vector>
#include <atomic>
#include "Timestamp.h"
#include <memory>
#include <mutex>
#include "CurrentThread.h"

class Channel;
class Poller;


//�¼�ѭ���� ��Ҫ������������ģ��Channel Poller(epoll�ĳ���)
class EventLoop:noncopyable
{
public:
    using Functor=std::function<void()>;//����������
    EventLoop();
    ~EventLoop();

    //�����¼�ѭ��
    void loop();
    //�˳��¼�ѭ��
    void quit();
    Timestamp pollReturnTime() const{return pollReturnTime_;};
    //�ڵ�ǰloop��ִ��cb
    void runInLoop(Functor cb);
    //��cb��������У�����loop���ڵ��̣߳�ִ��cb
    void queueInLoop(Functor cb);//�ڶ�����ת��loop�����߳���ִ��
    //��������loop���ڵ��̵߳�
    void wakeup();
    //EventLoop�ķ���=�� Poller�ķ���
    void updateChannel(Channel *Channel);//����poller����channel�¼�
    void removeChannel(Channel *Channel);
    //�ж�EventLoop�����Ƿ����Լ����߳�����
    bool hasChannel(Channel *channel);

    bool isInLoopThread()const {return threadId_==CurrentThread::tid();};//eventloop�Ƿ������������߳��� �Ƚ�id
private:
    void handleRead();//�����߳��� wake up
    void doPendingFunctors();//ִ�лص�    

    using ChannelList=std::vector<Channel*>;
    std::atomic_bool looping_;//ת��ԭ�Ӳ���(����ĳһ����ʱ�������)�Ĳ���ֵ �ײ�ͨ��CASʵ�ֵ� ��������loop�Ƿ�ѭ�������������
    std::atomic_bool quit_;//һ���������̵߳���eventloop��quit ��ʶ�˳�loopѭ��
    
    const pid_t threadId_;//��¼��ǰloop�����̵߳�id

    Timestamp pollReturnTime_;//poller���ط����¼���channels(activatechannels)��ʱ���
    std::unique_ptr<Poller> poller_;

    int wakeupFd_;//��Ҫ���ã���mainLoop��ȡһ�����û���channel��ͨ����ѯ�㷨ѡ��һ��subloop(��subreactor)��ͨ���ó�Ա����subloop����channel
    std::unique_ptr<Channel> wakeupChannel_;//�����������Ȥ�¼���wakefd

    ChannelList activateChannels_;
    //Channel *currentActivateChannel_;

    std::atomic_bool callingPendingFunctors_;//��ʶ��ǰloop�Ƿ�����Ҫִ�еĻص�����
    std::vector<Functor> pendingFunctors_;//�洢loop��Ҫִ�е����еĻص����� ������������߳�ϣ�������EventLoop�߳�ִ�еĺ���
    std::mutex mutex_;//����������������vector�������̰߳�ȫ����

};