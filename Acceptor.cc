#include "Acceptor.h"
#include <sys/types.h>          
#include <sys/socket.h>
#include "Logger.h"
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include "InetAddress.h"

static int createNonblocking()//非阻塞IO SOCK_CLOEXEC代表子进程能继承父进程所有fd 这里默认子进程关闭这些fd
{
    int sockfd=::socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0);//最后一个参数原来是IPPROTO_TCP
    if (sockfd<0)
    {
        LOG_FATAL("%s:%s:%d listen socket create err:%d \n",__FILE__,__FUNCTION__,__LINE__,errno);
    }        
}

Acceptor::Acceptor(EventLoop *loop,const InetAddress &listenAddr,bool reuseport)//tcpserver的参数
    :loop_(loop)//访问当前poller
    ,acceptSocket_(createNonblocking())
    ,acceptChannel_(loop,acceptSocket_.fd())//当前loop，listenfd
    ,listenning_(false)
{
    acceptSocket_.setReuseAddr(true);//更改tcp选项
    acceptSocket_.setReusePort(true);//创建套接字
    acceptSocket_.bindAddress(listenAddr);//bind套接字
    //TcpServer::start() Acceptor.listen 有新用户连接，要执行一个回调(connfd打包=>channel=>subloop)
    //baseLoop=>acceptChannel_(listenfd)=>
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead,this));//设置回调函数 给channel注册的读回调函数 acceptchannel只关心读事件 也就是只关心新用户连接
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();//不注册事件
    acceptChannel_.remove();//将channel从当前epoll中清除
}


void Acceptor::listen()
{
    listenning_=true;//开启监听
    acceptSocket_.listen();//listen
    acceptChannel_.enableReading();//将channel注册到=>poller(mainloop的poller)里面 poller来监听 如果有调用readcallback

}

//listenfd有事件发生了，就是有新用户连接了
//给acceptchannel注册的读回调函数
void Acceptor::handleRead()
{
    InetAddress peerAddr;//peerAddr客户端连接
    int connfd=acceptSocket_.accept(&peerAddr);//返回通信connfd(cfd) 
    if (connfd>=0)//执行回调函数
    {
        if (newConnectionCallback_)//TcpServer预先给acceptor设置的回调函数 里面包括新创建tcpconnection等
        {
            newConnectionCallback_(connfd,peerAddr);//轮询找到subloop，唤醒，分发当前的新客户端的Channel
        }
        else{
            ::close(connfd);//没有回调函数直接关闭
        }
    }
    else
    {
        LOG_ERROR("%s:%s:%d accept err:%d \n",__FILE__,__FUNCTION__,__LINE__,errno);
        if (errno==EMFILE)//文件描述符耗尽
        {
            LOG_ERROR("%s:%s:%d sockfd reached limit \n",__FILE__,__FUNCTION__,__LINE__);
        }
    }

}