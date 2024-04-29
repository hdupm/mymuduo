#pragma once
#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"
#include <memory>
#include <string>
#include <atomic>

//enable_share_from_this是允许获得当前对象的智能指针
class Channel;
class EventLoop;
class Socket;

/*
TcpServer=>Acceptor=>有一个新用户连接，通过accept函数拿到connfd
=>打包成TcpConnection 设置回调=>Channel=>Poller=>Channel的回调操作
*/

class TcpConnection:noncopyable,public std::enable_shared_from_this<TcpConnection>
{

public:
    TcpConnection(EventLoop *loop,const std::string &name,int sockfd,const InetAddress& localAddr,const InetAddress& peerAddr);////////??????
    ~TcpConnection();

    EventLoop* getLoop() const {return loop_;}
    const std::string& name() const {return name_;}
    const InetAddress& localAddress() const {return localAddr_;}
    const InetAddress& peerAddress() const {return peerAddr_;}
    bool connected() const {return state_==kConnected;}

    //发送数据 给客户端返回 向outbuffer发 直接提供string作为参数
    void send(const std::string &buf);
    //关闭连接
    void shutdown();

    // 设置链接回调函数，TcpServer::newConnection调用
    void setConnectionCallback(const ConnectionCallback &cb)
    {
    connectionCallback_ = cb;
    }
    // 设置消息回调函数，TcpServer::newConnection调用
    void setMessageCallback(const MessageCallback &cb)
    {
    messageCallback_ = cb;
    }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { 
    writeCompleteCallback_ = cb; 
    }
    // 设置关闭链接回调函数，TcpServer::newConnection调用
    // 传入参数cb = boost::bind(&TcpServer::removeConnection, this, _1)

    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
    { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }
    
    void setCloseCallback(const CloseCallback &cb)
    {
    closeCallback_ = cb;
    }

    // 连接建立，TcpServer::newConnection调用
    void connectEstablished(); // should be called only once

    // TcpServer::removeConnection调用，当TcpServer将当前connection对象从链接列表中移除后调用
    // 内部定义，但不会直接调用，放入Eventloop的functors列表当中
    // loop_->queueInLoop(boost::bind(&TcpConnection::connectDestroyed, conn));
    //连接销毁
    void connectDestroyed(); // should be called only once
    


private:
    enum StateE {kDisconnected,kConnecting,kConnected,kDisconnecting};//枚举连接状态
    void setState(StateE state){state_=state;}// 设置链接状态
    void handleRead(Timestamp receiveTime); // 当通道出现可读事件，会回调
    void handleWrite();
    void handleClose();                     // 链接关闭时回调
    void handleError();                     // 事件中可能会有错误
    
    
    void sendInLoop(const void* message, size_t len);
    void shutdownInLoop();

    EventLoop *loop_;//这里绝对不是baseLoop，因为TcpConnection都是在subLoop里面管理的
    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    //这里和Acceptor类似 Acceptor=>mainLoop TcpConnection=> subLoop
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;//当前主机ip端口
    const InetAddress peerAddr_;//对端ip端口

    ConnectionCallback connectionCallback_;//有新连接时的回调 无论是建立还是销毁都要调用这个
    MessageCallback messageCallback_;//有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_;//消息发送完成以后的回调
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;
    size_t highWaterMark_;//水位线，记录多少数据量到达水位线 防止应用发送数据过快底层TCP等来不及接收 默认64M

    Buffer inputBuffer_;//接收数据的缓冲区
    Buffer outputBuffer_;//发送数据的缓冲

};