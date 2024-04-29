#pragma once

/*
�û�ʹ��muduo��д����������
*/
#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"
#include "TcpConnection.h"
#include "Buffer.h"
#include <atomic>
#include <functional>
#include <memory>
#include <unordered_map> 
//������������ʹ�õ��� �Լ�дchatserverֻ�����TcpServer����
class TcpServer:noncopyable
{
public:
    using ThreadInitCallback=std::function<void(EventLoop*)>;
    enum Option
    {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop *loop,const InetAddress &listenAddr,const std::string &nameArg,Option option=kNoReusePort);
    ~TcpServer();
    void setThreadInitcallback(const ThreadInitCallback &cb) {threadInitCallback_=cb;}
    void setConnectionCallback(const ConnectionCallback &cb) {connectionCallback_=cb;}
    void setMessageCallback(const MessageCallback &cb) {messageCallback_=cb;}
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) {writeCompleteCallback_=cb;}

    //���õײ� subloop�ĸ���
    void setThreadNum(int numThreads);

    //��������������
    void start();

private:
    void newConnection(int sockfd,const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    using ConnectionMap=std::unordered_map<std::string,TcpConnectionPtr>;

    EventLoop *loop_;//baseLoop �û������loop ��mainloop ��Ҫ����acceptor
    const std::string ipPort_;//���������ip�˿ں�
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_; //����ָ���й���Դ ������mainLoop��������Ǽ����������¼�

    std::shared_ptr<EventLoopThreadPool> threadPool_;//one loop per thread
    //�û��Զ���Ļص����� �û�����tcpserver�ٴ���eventloop

    ConnectionCallback connectionCallback_;//��������ʱ�Ļص�
    MessageCallback messageCallback_;//�ж�д��Ϣʱ�Ļص�
    WriteCompleteCallback writeCompleteCallback_;//��Ϣ��������Ժ�Ļص�

    ThreadInitCallback threadInitCallback_;//loop�̳߳�ʼ���Ļص�
    std::atomic_int started_;
    int nextConnId_;
    ConnectionMap connections_;//�������е�����
    
};