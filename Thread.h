//thread��ֻ��ע��һ���߳�
#pragma once
#include "noncopyable.h"
#include <functional>
#include <thread>//��c++���߳��� c���߳���Ϸ���
#include <memory>
#include <unistd.h>
#include <atomic>
#include <string>

class Thread:noncopyable
{
public:
    using ThreadFunc=std::function<void()>;//����к������� ��ô�����ð����ͺ�������

    explicit Thread(ThreadFunc,const std::string& name=std::string());
    ~Thread();
    void start();
    void join();
   
    bool started() const{return started_;}
    pid_t tid() const {return tid_;}//�����������̺߳�
    const std::string& name() const {return name_;}

    static int numCreated() {return numCreated_;}
private:
    void setDefaultName();
    bool started_;
    bool joined_;
    //����thread��linux��pthreadһ����api���ú��߳�ֱ�ӿ�ʼ��������������Ҫ�����̶߳���������ʱ�� ��Ҫ����ָ���װ
    std::shared_ptr<std::thread>thread_;
    pid_t tid_;
    ThreadFunc func_;//�洢�̺߳���
    std::string name_;//�߳�����
    static std::atomic_int numCreated_;
};