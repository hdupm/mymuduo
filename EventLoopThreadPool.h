//�̳߳� �ײ����eventloopthread
#pragma once 
#include "noncopyable.h"
#include <functional>
#include <string>
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool:noncopyable
{
public:
    using ThreadInitCallback=std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop *baseLoop,const std::string &nameArg);
    ~EventLoopThreadPool();//����������ʲô

    void setThreadNum(int numThreads){numThreads_=numThreads;}
    void start(const ThreadInitCallback &cb=ThreadInitCallback());//�����¼�ѭ���߳�
    //��������ڶ��߳��У�baseloop_Ĭ������ѯ��ʽ����channel��subloop
    EventLoop* getNextLoop();//mainloop(baseloop)
    std::vector<EventLoop*> getAllLoops();//��������loops
    bool started() const{return started_;}
    const std::string name() const {return name_;}

private:
    EventLoop *baseLoop_;//�ʼ�û�������loop mainloop
    std::string name_;
    bool started_;
    int numThreads_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;//�������д����¼�ѭ�����߳�
    std::vector<EventLoop*> loops_;//�¼��߳�eventloop��ָ��


};