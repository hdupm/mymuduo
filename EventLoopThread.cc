#include "EventLoopThread.h"
#include "EventLoop.h"


EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
                                 const std::string &name)
    :loop_(nullptr)
    ,exiting_(false)
    ,thread_(std::bind(&EventLoopThread::threadFunc,this),name)//�󶨻ص�����
    ,mutex_()
    ,cond_()
    ,callback_(cb)
{

}

EventLoopThread::~EventLoopThread()
{
    exiting_=true;
    if (loop_!=nullptr)
    {
        loop_->quit();//�߳��˳�
        thread_.join();
    }
}
EventLoop* EventLoopThread::startLoop()//����ѭ�� �õ����߳�loopָ��
{
    thread_.start();//�����ײ�����߳� ִ���·����ײ��̵߳Ļص�����threadFunc
    EventLoop *loop=nullptr;//��Ҫ��startִ������ص�������� loopָ��ָ�������ܼ���ִ��
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while(loop_==nullptr)//˵�����̻߳�δִ����
        {
            cond_.wait(lock);//�ȴ�������
        }
        loop=loop_;
    }
    return loop;//�������߳����浥�����е�loop����

}

//����������������ڵ��������߳��������е�
void EventLoopThread::threadFunc()//�̺߳��� ����loop
{
    EventLoop loop;//����һ��������event loop����������߳���һһ��Ӧ�� one loop per thread ����ִ��loop���߳�
    if (callback_)//����лص����� ��ӦTcpserver��ThreadInitCallback���� ���Զ�loop���г�ʼ������
    {
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_=&loop;//�ú�����Ա����
        cond_.notify_one();//��������֪ͨһ��
    }
    
    loop.loop();//EventLoop loop����=>����Poller.poll loop��һֱѭ������
    //���뵽�������˵��������Ҫ�ر���
    std::unique_lock<std::mutex> lock(mutex_);//�ͷ���
    loop_=nullptr;

}