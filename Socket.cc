#include "Socket.h"
#include "Logger.h"
#include "InetAddress.h"
#include <unistd.h>
#include <strings.h>
#include <netinet/tcp.h>
#include <sys/types.h>    
#include <sys/socket.h>

Socket::~Socket()
{
    close(sockfd_);
}

void Socket::bindAddress(const InetAddress &localaddr)
{
    if (0!=::bind(sockfd_,(sockaddr*)localaddr.getSockAddr(),sizeof(sockaddr_in)))
    {
        LOG_FATAL("bind sockfd:%d fail \n",sockfd_); 
    }
}

void Socket::listen()
{
    if (0!=::listen(sockfd_,1024))
    {
        LOG_FATAL("listen sockfd:%d fail \n",sockfd_); 
    }
}

int Socket::accept(InetAddress *peeraddr)
{
    /*
    1、accept函数的参数不合法
    2、对返回的connfd没有设置非阻塞
    reactor模型 one loop per thread
    poller+non-blocking IO
    */
    sockaddr_in addr;
    socklen_t len=sizeof addr;//传入参数需要初始化大小
    bzero(&addr,sizeof addr);
    int connfd=::accept4(sockfd_,(sockaddr*)&addr,&len,SOCK_NONBLOCK|SOCK_CLOEXEC);//给accept设置标志位
    if (connfd>=0)
    {
        peeraddr->setSockAddr(addr);//客户端通过输出参数传出
    }
    return connfd;//返回新连接描述符
}

void Socket::shutdownWrite()//关闭写端
{
    if (::shutdown(sockfd_,SHUT_WR)<0)
    {
        LOG_ERROR("shutdownWrite error");
    }
}

//socket协议选项
void Socket::setTcpNoDelay(bool on)//协议级别
{
    int optval=on?1:0;
    ::setsockopt(sockfd_,IPPROTO_TCP,TCP_NODELAY,&optval,sizeof optval);
}
void Socket::setReuseAddr(bool on)//socket级别
{
    int optval=on?1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof optval);
}
void Socket::setReusePort(bool on)
{
    int optval=on?1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEPORT,&optval,sizeof optval);
}
void Socket::setKeepAlive(bool on)
{
    int optval=on?1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_KEEPALIVE,&optval,sizeof optval);
}

