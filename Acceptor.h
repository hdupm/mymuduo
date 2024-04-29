//accept运行在baseloop/mainloop
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
    Acceptor(EventLoop *loop,const InetAddress &listenAddr,bool reuseport);//tcpserver的参数
    ~Acceptor();
    void setNewConnectionCallback(const NewConnectionCallback &cb)//这里传入Tcpserver的newConnection函数
    {
        newConnectionCallback_=cb;//可以用右值引用move

    }

    bool listenning() const {return listenning_;}
    void listen();

private:
    void handleRead();
    
    EventLoop *loop_;//Acceptor用的就是用户定义的那个baseLoop，也称作mainLoop
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listenning_;
    
};
