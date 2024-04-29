#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"
#include <memory>

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop,const std::string &nameArg)
    :baseLoop_(baseLoop)
    ,name_(nameArg)
    ,started_(false)
    ,numThreads_(0)
    ,next_(0)//��ѯ�±�
{}

EventLoopThreadPool::~EventLoopThreadPool()//����������ʲô ע�ⲻ��Ҫ��עvector��ָ�뼴�ⲿ��Դ��Ҫdelete ��Ϊ�̰߳󶨵�loop����ջ�ϵĶ���
{}

void EventLoopThreadPool::start(const ThreadInitCallback &cb)//�����¼�ѭ���߳� �����ϼ�TcpServer�����Ļص���������ʼ���߳�
{
    started_=true;
    for (int i=0;i<numThreads_;++i)//�����߳���Ŀ�����߳�
    {
        char buf[name_.size()+32];//�̳߳�+�±������Ϊ�ײ��߳�����
        snprintf(buf,sizeof buf,"%s%d",name_.c_str(),i);
        EventLoopThread *t=new EventLoopThread(cb,buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->startLoop());//����ֻ�Ǵ��� �����ǵײ㴴���̣߳���һ���µ�EventLoop�������ظ�loop�ĵ�ַ
    }
    //����̳߳���ĿΪ0˵�����������ֻ��һ�����̣߳�������baseloop
    if (numThreads_==0&&cb)
    {
        cb(baseLoop_);//ִ���û����õĻص�
    }
}
//��������ڶ��߳��У�baseloop_Ĭ������ѯ��ʽ����channel��subloop
EventLoop* EventLoopThreadPool::getNextLoop()//mainloop(baseloop) ͨ����ѯ��ʽ�����߳�ȡloop
{
    EventLoop *loop=baseLoop_;//ָ��mainloop ���û�еײ������ôloop����baseloop
    if (!loops_.empty())//ͨ����ѯ��ȡ��һ�������¼���loop
    {
        loop=loops_[next_];//next_��ʼ��Ϊ0 ��next_��Ӧ���̸߳�ֵ��loop
        ++next_;
        if (next_>=loops_.size())
        {
            next_=0;
        }
    }
    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()//��������loops
{
    if (loops_.empty())//˵��ֻ��baseloop
    {
        return std::vector<EventLoop*>(1,baseLoop_);
    }
    else
    {
        loops_;///////////////////////
    }
}