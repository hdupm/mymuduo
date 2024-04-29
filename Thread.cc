#include "Thread.h"
#include "CurrentThread.h"
#include <semaphore.h>

std::atomic_int Thread::numCreated_(0);//��̬��Ա������Ҫ�����ⵥ�����г�ʼ�� ʹ�ñ�׼ָ�����캯��
Thread::Thread(ThreadFunc func,const std::string &name)
    :started_(false)
    ,joined_(false)
    ,tid_(0)
    ,func_(std::move(func))//��func�ײ���Դ����Ա����
    ,name_(name)
{
    setDefaultName();
}
Thread::~Thread()
{
    if (started_&&!joined_) //detach�൱�����óɷ����̣߳�����ػ��߳� ���߳̽������ػ��߳��Զ����� ������ֹ¶��߳� ��join��detach����ͬʱִ�� ��Ϊjoin����ͨ�Ĺ����߳�
    {
        thread_->detach();//thread���ṩ�����÷����̵߳ķ���
    }
}
void Thread::start()//һ��Thread���󣬼�¼�ľ���һ�����̵߳���ϸ��Ϣ
{
    started_=true;
    sem_t sem;//�ź�����Դ
    sem_init(&sem,false,0);
    //�����߳�
    thread_=std::shared_ptr<std::thread>(new std::thread([&](){ //lambda���ʽ �����̶߳��󲢴��̺߳��� �����÷�ʽ&�����ⲿthread���� ���Է��ʳ�Ա����
        //��ȡ�̵߳�tidֵ Ҳ�����߳�ִ�еĵ�һ��
        tid_=CurrentThread::tid();
        //�ź�����Դ+1
        sem_post(&sem);
        //����һ�����̣߳�ר��ִ�и��̺߳��� ִ�е���һ���ص�EventLoopThread::threadFunc
        func_();
    }));

    //�������ȴ���ȡ�����´������´������̵߳�tidֵ
    sem_wait(&sem);//���û���źŻὫstart����ס
    
}
void Thread::join()
{
    joined_=true;
    thread_->join();
}

void Thread::setDefaultName()
{
    int num=++numCreated_;
    if (name_.empty())
    {
        char buf[32]={0};
        snprintf(buf,sizeof buf,"Thread%d",num);
        name_=buf;
    }

}