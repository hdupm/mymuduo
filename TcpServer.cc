#include "TcpServer.h"
#include "Logger.h"
#include "TcpConnection.h"
#include <functional>
#include <strings.h>


static EventLoop* CheckLoopNotNull(EventLoop *loop)
{
    if(loop==nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null!\n",__FILE__,__FUNCTION__,__LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop,
                const InetAddress &listenAddr,
                const std::string &nameArg,
                Option option)//定义的时候是默认值，因为声明的时候已经给了
                :loop_(loop)//不允许用户传空loop
                ,ipPort_(listenAddr.toIpPort())//用户传入ip和端口
                ,name_(nameArg)
                ,acceptor_(new Acceptor(loop,listenAddr,option==kReusePort)) //仅acceptor在mainloop中 里面相当于创建socket过程
                ,threadPool_(new EventLoopThreadPool(loop,name_))
                ,connectionCallback_()//用户如果没有注册回调函数，那么执行默认的 该回调函数输出本地地址，对端地址，连接状态
                ,messageCallback_()//将buffer中的读索引和写索引重置
                ,nextConnId_(1)//下一个建立的连接ID位1
                ,started_(0)
{
    //当有新用户连接时，会执行TcpServer::newConnection回调(传入acceptor，acceptor::handleread()里面可使用该回调函数)
    //绑定器 传入的是上级用户设置的回调 作为后序回调函数的传入参数cb
    //这个是设置acceptor的channel的回调函数setReadCallback，也就是面对新连接的读事件所调用的回调函数
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection,this,
        std::placeholders::_1,std::placeholders::_2)); //绑定器第一个是传入自己，因为acceptor的回调函数含有两个参数，用两个占位符
}

TcpServer::~TcpServer()
{
    for (auto &item:connections_) //将连接的智能指针释放 不再用强智能指针管理资源 而是用局部对象管理 否则无法释放 最后删除连接
    {
        TcpConnectionPtr conn(item.second);//这个局部的shared_ptr智能指针对象 出右括号可以自动释放new出来的TcpConnection对象资源了   
        item.second.reset();//将连接的智能指针释放

        //销毁连接
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed,conn)
        );
    }
}

//设置底层subloop的个数
void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

//开启服务器监听
void TcpServer::start()
{
    if (started_++==0) //防止一个TcpServer对象被start多次
    {
        threadPool_->start(threadInitCallback_);//启动底层线程池
        loop_->runInLoop(std::bind(&Acceptor::listen,acceptor_.get()));//启动listen Acceptor::listen是将acceptchannel注册在了baseloop上
    }
}

//有一个新的客户端的连接，acceptor会执行这个回调操作 仅在mainloop里处理因此不涉及线程安全
//设置连接名字、创建连接、给连接绑定回调
void TcpServer::newConnection(int sockfd,const InetAddress &peerAddr)
{
    //轮询算法，选择一个sunLoop，来管理channel
    EventLoop *ioLoop=threadPool_->getNextLoop();
    char buf[64]={0};
    snprintf(buf,sizeof buf,"-%s#%d",ipPort_.c_str(),nextConnId_);//表示连接名称
    ++nextConnId_;
    std::string connName=name_+buf;//连接名字

    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s \n",
        name_.c_str(),connName.c_str(),peerAddr.toIpPort().c_str());

    //通过sockfd获取其绑定的本机的ip地址和端口信息
    sockaddr_in local;
    ::bzero(&local,sizeof local);
    socklen_t addrlen=sizeof local;
    if (::getsockname(sockfd,(sockaddr*)&local,&addrlen)<0)
    {
        LOG_ERROR("sockets::getLocalAddr");
    }
    InetAddress localAddr(local);

    //根据连接成功的sockfd，创建TcpConnection连接对象
    TcpConnectionPtr conn(new TcpConnection(
                        ioLoop,
                        connName,
                        sockfd, //通过这些信息创建底层Socket Channel对象
                        localAddr,
                        peerAddr));
    connections_[connName]=conn;
    //下面的回调都是用户设置给TcpServer=>TcpConnection=>Channel=>注册Poller=>notify channel调用回调
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    //设置了如何关闭连接的回调 conn->shutDown()
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection,this,std::placeholders::_1)
    );

    //直接调用 注意TcpConnection是在主线程中的，但下面的是子线程执行
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished,conn));


}
void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(
        std::bind(&TcpServer::removeConnectionInLoop,this,conn)
    );
}
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n",name_.c_str(),conn->name().c_str());
    connections_.erase(conn->name());//从map表中删除 conn的名字作为map键
    EventLoop *ioLoop=conn->getLoop();
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed,conn)
    );

}