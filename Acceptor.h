//accept������baseloop/mainloop
#pragma once
#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"
#include <functional>

class EventLoop;
class InetAddress;

class Acceptor:noncopyable
{
public:
    using NewConnectionCallback=std::function<void(int sockfd,const InetAddress&)>;
    Acceptor(EventLoop *loop,const InetAddress &listenAddr,bool reuseport);//tcpserver�Ĳ���
    ~Acceptor();
    void setNewConnectionCallback(const NewConnectionCallback &cb)//���ﴫ��Tcpserver��newConnection����
    {
        newConnectionCallback_=cb;//��������ֵ����move

    }

    bool listenning() const {return listenning_;}
    void listen();

private:
    void handleRead();
    
    EventLoop *loop_;//Acceptor�õľ����û�������Ǹ�baseLoop��Ҳ����mainLoop
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listenning_;
    
};
