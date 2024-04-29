#include "EventLoop.h"
#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include "Logger.h"
#include <errno.h>
#include "Poller.h"
#include "Channel.h"

//��ֹһ���̴߳������EventLoop �����󱻴��������� ��ָ����� __thread��thread_local�Ļ��� �����߳����� ������Ӿ���ȫ�ֱ����������̹߳�������ÿ���̶߳����������
__thread EventLoop *t_loopInThisThread=nullptr;

//����Ĭ�ϵ�Poller IO���ýӿڵĳ�ʱʱ�� 10s Ҳ����epoll_waitÿ10����ѯһ�β����ؽ�� ����־��ӡҲ��10s���õ�
const int kPollTimeMs=10000;

//����wakeupfd������notify����subReactor����������channel(������ѯ��ʽ)
int createEventfd()//����ϵͳ�ӿ�
{
    int evtfd=::eventfd(0,EFD_NONBLOCK|EFD_CLOEXEC);//����wakeup fd������֪ͨ˯����eventloop���������û���channel��Ҫ����
    if (evtfd<0)
    {
        LOG_FATAL("eventfd error:%d",errno);
    }
    return evtfd;    
}

EventLoop::EventLoop()
    :looping_(false)
    ,quit_(false)
    ,callingPendingFunctors_(false)
    ,threadId_(CurrentThread::tid())
    ,poller_(Poller::newDefaultPoller(this))//��ȡĬ��poller
    ,wakeupFd_(createEventfd())
    ,wakeupChannel_(new Channel(this,wakeupFd_)) //ÿ��subreactor��������wakeupfd ��mainreactor notify wakeupfd����Ӧ��loop�ͱ�����
    //,currentActivateChannel_(nullptr)
{
    LOG_DEBUG("EventLoop created %p in thread %d \n",this,threadId_);
    if (t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n",t_loopInThisThread,threadId_);
    }
    else
    {
        t_loopInThisThread=this;
    }

    //����wakeupfd���¼������Լ�����ʱ���Ļص�����(����subreactor/eventloop) ���һֱû�����飬poll���᷵�ء�������ʱ����������(�����µ�����)��������poll����
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead,this)); //�����ͺ�������
    //ÿһ��eventloop��������wakeupchannel��EPOLLIN���¼���
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()//��Դ����
{
    wakeupChannel_->disableAll();//�������¼�����Ȥ
    wakeupChannel_->remove();//��poller���Ƴ�
    ::close(wakeupFd_);
    t_loopInThisThread=nullptr;
}

//�����¼�ѭ��(�����ײ�poller��ִ��poll����)
void EventLoop::loop()
{
    looping_=true;//����ѭ��
    quit_=false;//δ�˳�

    LOG_INFO("EventLoop %p start looping\n",this);
    
    while(!quit_)
    {
        activateChannels_.clear();
        //��������fd һ����client��fd(�ͻ���ͨ��)��һ��wakeupfd(mainloop����subloop)
        pollReturnTime_=poller_->poll(kPollTimeMs,&activateChannels_);//���볬ʱʱ��
        for (Channel *channel:activateChannels_)//�õ������¼���channel��̬����activateChannels_
        {   
            //poller������Щchannel�����¼��ˣ�Ȼ���ϱ���EventLoop��֪ͨchannel������Ӧ���¼�
            channel->handleEvent(pollReturnTime_);
        }
        //ִ�е�ǰEventLoop�¼�ѭ����Ҫ����Ļص�����
        /*
        IO�߳� mainLoop accept���� ����fd��=�����channel ���Ѳ��ַ���subloop
        mainloop����ע��һ���ص�cb (��Ҫsubloop��ִ��) wakeup subloop��ִ������ķ�����ִ��֮ǰmainloopע���cb����
        ��������µ�channel        
        */
        doPendingFunctors();//ִ�лص�
        
    }
    LOG_INFO("EventLoop %p stop looping.\n",this);
    looping_=false;
}

//�˳��¼�ѭ����������� 1.loop���Լ����߳��е���quit 2.�ڷ�loop���߳��е���loop��quit
void EventLoop::quit()
{
    quit_=true;
    //!isInLoopThread()˵�������Լ��߳���quit ������߳���quit_��true���Զ����������whileѭ��
    //������������߳��е��õ�quit��������һ��subloop(worker)�У�������mainLoop(IO)��quit
    /*
                        mainLoop (ÿһ��loop����һ��wakeupfd�ܻ��Ѷ�Ӧloop)
        =============================================������-�����ߵ��̰߳�ȫ���� muduo��û�� main��subͨ��wakeupfdֱ��ͨ��

    subLoop1        subLoop2            subLoop3
    
    */
    if (!isInLoopThread())
    {
        wakeup();//��Ҫ�������߳�mainloop
    }
}

//�ڵ�ǰloop��ִ��cb
void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread())//�ڵ�ǰ��loop�߳���ִ��cb(cb����channel�й�)
    {
        cb();
    }
    else//�ڷǵ�ǰloop�߳���ִ��cb������ҪLoop�����̣߳�ִ��cb
    {
        queueInLoop(cb);
    }
}

//��cb��������У�����loop���ڵ��̣߳�ִ��cb(Ҳ˵��cb���ڵ�ǰloop��)
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);//������
        pendingFunctors_.emplace_back(cb);//vector�ײ�ĩβֱ�ӹ���cb�������ǿ�������
    }

    //loop���Ǵ��̵߳ģ���Ҫ������Ӧ�ģ���Ҫִ�е�����ص�������loop���߳���
    //callpendingfunctors_����˼�ǣ���ǰloop����ִ�лص�������loop�����µĻص� Ϊ�˷�ֹ������Ҫwakeup���̼߳���ִ������
    if (!isInLoopThread()||callingPendingFunctors_)
    {
        wakeup();//����loop�����߳�
    }
}

//��������loop���ڵ��̵߳� ��wakeupfd_дһ������ ����wakeupchannel��Ӧ�������¼�����ǰloop�߳̾ͻᱻ����
void EventLoop::wakeup()
{
    uint64_t one=1;
    ssize_t n=write(wakeupFd_,&one,sizeof one) ;
    if (n!=sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n",n);
    }

}

//EventLoop�ķ���=�� Poller�ķ���
void EventLoop::updateChannel(Channel *channel)//����poller�ķ�������channel�¼�
{
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}
//�ж�EventLoop�����Ƿ����Լ����߳�����
bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors()//���Ѻ�ִ�лص�
{
    std::vector<Functor> functors;
    callingPendingFunctors_=true;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);//�ͷ�pendingFunctors_������mainloop�����ȴ� ����ʵ��ִ�лص���mainloop��ǰloopд�ص���ͬʱ����
    }

    for (const Functor &functor:functors)
    {
        functor();//ִ�е�ǰloop��Ҫִ�еĻص�����
    }
    callingPendingFunctors_ = false;    
}    

void EventLoop::handleRead() //������ȡwakeupfd�Ķ��¼�
{
  uint64_t one = 1;//buf 8�ֽ�
  ssize_t n = read(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
    LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8",n) ;
  }
}