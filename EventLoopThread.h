//��loop��thread ��loop������thread ��һ�̴߳���һ��eventloop
#pragma once
#include "noncopyable.h"
#include <functional>
#include "Thread.h"
#include <mutex>
#include <condition_variable>//��������
#include <string>

class EventLoop;

class EventLoopThread:noncopyable
{
public:
    using ThreadInitCallback=std::function<void(EventLoop*)>;//����eventloop����г�ʼ������

    EventLoopThread(const ThreadInitCallback &cb,const std::string &name= std::string());//����ֵ���ܶ�� ����ʱ�������Ͳ��ø���
    ~EventLoopThread();
    EventLoop* startLoop();//����ѭ��

private:
    void threadFunc();//�̺߳��� ����loop
    EventLoop *loop_;
    bool exiting_;//�Ƿ��˳�ѭ��
    Thread  thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;
};