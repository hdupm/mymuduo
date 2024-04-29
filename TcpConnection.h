#pragma once
#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"
#include <memory>
#include <string>
#include <atomic>

//enable_share_from_this�������õ�ǰ���������ָ��
class Channel;
class EventLoop;
class Socket;

/*
TcpServer=>Acceptor=>��һ�����û����ӣ�ͨ��accept�����õ�connfd
=>�����TcpConnection ���ûص�=>Channel=>Poller=>Channel�Ļص�����
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

    //�������� ���ͻ��˷��� ��outbuffer�� ֱ���ṩstring��Ϊ����
    void send(const std::string &buf);
    //�ر�����
    void shutdown();

    // �������ӻص�������TcpServer::newConnection����
    void setConnectionCallback(const ConnectionCallback &cb)
    {
    connectionCallback_ = cb;
    }
    // ������Ϣ�ص�������TcpServer::newConnection����
    void setMessageCallback(const MessageCallback &cb)
    {
    messageCallback_ = cb;
    }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { 
    writeCompleteCallback_ = cb; 
    }
    // ���ùر����ӻص�������TcpServer::newConnection����
    // �������cb = boost::bind(&TcpServer::removeConnection, this, _1)

    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
    { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }
    
    void setCloseCallback(const CloseCallback &cb)
    {
    closeCallback_ = cb;
    }

    // ���ӽ�����TcpServer::newConnection����
    void connectEstablished(); // should be called only once

    // TcpServer::removeConnection���ã���TcpServer����ǰconnection����������б����Ƴ������
    // �ڲ����壬������ֱ�ӵ��ã�����Eventloop��functors�б���
    // loop_->queueInLoop(boost::bind(&TcpConnection::connectDestroyed, conn));
    //��������
    void connectDestroyed(); // should be called only once
    


private:
    enum StateE {kDisconnected,kConnecting,kConnected,kDisconnecting};//ö������״̬
    void setState(StateE state){state_=state;}// ��������״̬
    void handleRead(Timestamp receiveTime); // ��ͨ�����ֿɶ��¼�����ص�
    void handleWrite();
    void handleClose();                     // ���ӹر�ʱ�ص�
    void handleError();                     // �¼��п��ܻ��д���
    
    
    void sendInLoop(const void* message, size_t len);
    void shutdownInLoop();

    EventLoop *loop_;//������Բ���baseLoop����ΪTcpConnection������subLoop��������
    const std::string name_;
    std::atomic_int state_;
    bool reading_;

    //�����Acceptor���� Acceptor=>mainLoop TcpConnection=> subLoop
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;//��ǰ����ip�˿�
    const InetAddress peerAddr_;//�Զ�ip�˿�

    ConnectionCallback connectionCallback_;//��������ʱ�Ļص� �����ǽ����������ٶ�Ҫ�������
    MessageCallback messageCallback_;//�ж�д��Ϣʱ�Ļص�
    WriteCompleteCallback writeCompleteCallback_;//��Ϣ��������Ժ�Ļص�
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;
    size_t highWaterMark_;//ˮλ�ߣ���¼��������������ˮλ�� ��ֹӦ�÷������ݹ���ײ�TCP������������ Ĭ��64M

    Buffer inputBuffer_;//�������ݵĻ�����
    Buffer outputBuffer_;//�������ݵĻ���

};