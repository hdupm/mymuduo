#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

#include <functional>
#include <errno.h>
#include <netinet/tcp.h>
#include <sys/types.h>    
#include <sys/socket.h>
#include <strings.h>

static EventLoop* CheckLoopNotNull(EventLoop *loop)//静态防止命名冲突
{
    if(loop==nullptr)
    {
        LOG_FATAL("%s:%s:%d TcpConnection Loop is null!\n",__FILE__,__FUNCTION__,__LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop
            ,const std::string &nameArg
            ,int sockfd
            ,const InetAddress& localAddr
            ,const InetAddress& peerAddr)
            :loop_(CheckLoopNotNull(loop))
            ,name_(nameArg)
            ,state_(kConnecting)
            ,reading_(true)
            ,socket_(new Socket(sockfd))
            ,channel_(new Channel(loop,sockfd))
            ,localAddr_(localAddr)
            ,peerAddr_(peerAddr)
            ,highWaterMark_(64*1024*1024)//64M 超过可以设置操作 比如停止发送
{
    //给channel设置相应的回调函数，poller通知channel感兴趣的事件发生了，channel会回调相应的操作函数
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead,this,std::placeholders::_1)
    );
    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite,this)
    );
        
    
    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose,this)
    );
    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError,this)
    );

    LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n",name_.c_str(),sockfd);
    socket_->setKeepAlive(true);

}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::Dtor[%s] at fd=%d state=%d \n",name_.c_str(),channel_->fd(),(int)state_);//枚举类型需要主动转成整型
}

void TcpConnection::send(const std::string &buf)
{
    if (state_==kConnected)
    {
        if (loop_->isInLoopThread())//当前loop在对应线程中
        {
            sendInLoop(buf.c_str(),buf.size());            
        }
        else//执行回调
        {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop,this,buf.c_str(),buf.size()));
        }
    }
}

/*
发送数据 应用写的快，而内核发送数据慢，需要把待发送数据写入缓冲区，而且设置了水位回调
*/
void TcpConnection::sendInLoop(const void* data, size_t len)
{
    ssize_t nwrote=0;
    size_t remaining=len;//没法送完的数据
    bool faultError=false;

    //之前调用过改connection的shutdown，不能再进行发送了
    if (state_==kDisconnected)
    {
        LOG_ERROR("disconnected,give up writing!");
        return;
    }

    //表示channel_第一次开始写数据，而且缓冲区没有待发送数据     
    if (!channel_->isWriting() && outputBuffer_.readableBytes()==0)
    {
        nwrote=::write(channel_->fd(),data,len);//实际写入个数
        if (nwrote>=0)
        {
            remaining=len-nwrote;//需要写入-实际写入
            if (remaining==0 && writeCompleteCallback_)
            {
                //既然在这里数据全部发送完成，就不用给channel设置epollout事件了
                loop_->queueInLoop(
                    std::bind(writeCompleteCallback_,shared_from_this())
                );
            }
            
        }
        else //nwrote<0
        {
            nwrote=0;
            if (errno!=EWOULDBLOCK)//是非阻塞
            {
                LOG_ERROR("TcpConnection::sendInLoop");
                if (errno==EPIPE||errno==ECONNRESET)//SIGPIPE RESET 接收到对端重置 连接错误
                {
                    faultError=true;
                }

            }
        }
    }

    //说明当前这一次write并没有把数据全部发送出去，剩余的数据需要保存到缓冲区当中，然后给channel注册epollout事件
    //(因为是LT模式)poller发现tcp的发送缓冲区有空间，会通知相应的sock-channel，调用channel->writecallback_回调方法
    //也就是调用TcpConnection::handleWrite方法，把发送缓冲区的数据全部发送完成
    //数据量较大情况，无法一次性将所有数据写入到channel中
    if (!faultError && remaining>0)
    {
        //目前发送缓冲区剩余的待发送数据的长度
        size_t oldLen=outputBuffer_.readableBytes();
        if (oldLen+remaining >= highWaterMark_ && oldLen < highWaterMark_&& highWaterMarkCallback_)
        {
            loop_->queueInLoop(
                std::bind(highWaterMarkCallback_,shared_from_this(),oldLen+remaining)
            );
        }
        outputBuffer_.append((char*)data+nwrote,remaining);
        if (!channel_->isWriting())
        {
            channel_->enableWriting();//这里一定要注册channel的写事件，否则poller不会给channel通知epollout
        }
    }
}

//连接建立 TcpConnection给channel设置相应的回调
void TcpConnection::connectEstablished() // should be called only once
{
    setState(kConnected);
    channel_->tie(shared_from_this());//记录TcpConnection状态
    channel_->enableReading();//向poller注册channel的epollin事件
    //新连接建立 执行回调 
    connectionCallback_(shared_from_this());
}   

// TcpServer::removeConnection调用，当TcpServer将当前connection对象从链接列表中移除后调用
// 内部定义，但不会直接调用，放入Eventloop的functors列表当中
// loop_->queueInLoop(boost::bind(&TcpConnection::connectDestroyed, conn));
// 连接销毁
void TcpConnection::connectDestroyed() // should be called only once
{
    if (state_==kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll();//把channel所有感兴趣事件从poller中del掉
        connectionCallback_(shared_from_this());
    }
    channel_->remove();//把channel从poller中删除掉
}

//关闭连接 这是用户调用，但此时底层有可能又写事件没结束
//muduo中每一个loop都要在所在线程中执行
void TcpConnection::shutdown()
{
    if (state_==kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(
            std::bind(&TcpConnection::shutdownInLoop,this)
        );
    }
}

//TcpConnection::shutdown()->shutdownInLoop->socket关闭写段->epoll通知channel关闭事件->回调TcpConnection::handleClose()方法
void TcpConnection::shutdownInLoop()
{
    if (!channel_->isWriting())//channel_没有写事件 说明outputBuffer中的数据已经全部发送完成
    {
        socket_->shutdownWrite();//关闭写端 触发epoll中的EPOLLHUP(无需注册)
    }
}

void TcpConnection::handleRead(Timestamp receiveTime)// 当通道出现可读事件，会回调
{
    int savedErrno=0;
    ssize_t n=inputBuffer_.readFd(channel_->fd(),&savedErrno);
    if (n>0)
    {
        //已建立连接的用户，有可读事件发生了，调用用户传入的回调操作onMessage
        messageCallback_(shared_from_this(),&inputBuffer_,receiveTime);  //shared_from_this是获取当前tcpconnection对象的智能指针
    }
    else if (n==0)
    {
        handleClose();
    }
    else
    {
        errno=savedErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }

}
void TcpConnection::handleWrite()
{
    if (channel_->isWriting()) //判断channel是否可写
    {
        int savedErrno=0;
        ssize_t n=outputBuffer_.writeFd(channel_->fd(),&savedErrno);
        if (n>0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes()==0)//说明发送完成
            {
                channel_->disableWriting();
                if (writeCompleteCallback_)
                {
                    //唤醒loop_对应的thread线程，执行回调
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_,shared_from_this())
                    );
                }
                if (state_==kDisconnecting)
                {
                    shutdownInLoop();//在当前loop中将TcpConnection删除掉
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else//channel对写事件不感兴趣
    {
        LOG_ERROR("TcpConnection fd=%d is down,no more writing \n",channel_->fd());
    }
}

//poller=>channel::closeCallback=>TC中预置的回调函数TcpConnection::handleClose
void TcpConnection::handleClose()                    // 链接关闭时回调
{
    LOG_INFO("TcpConnection::handleClose fd=%d state=%d \n",channel_->fd(),(int)state_);
    setState(kDisconnected);
    channel_->disableAll();
    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr);//执行连接关闭的回调
    closeCallback_(connPtr);//关闭连接的回调 执行的是TcpServer::removeConnection回调方法
}

void TcpConnection::handleError()                    // 事件中可能会有错误
{
    int optval;
    socklen_t optlen=sizeof optval;
    int err=0;
    if (::getsockopt(channel_->fd(),SOL_SOCKET,SO_ERROR,&optval,&optlen)<0)
    {
        err=errno;
    }
    else
    {
        err=optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d \n",name_.c_str(),err);   

}