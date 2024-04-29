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
    1��accept�����Ĳ������Ϸ�
    2���Է��ص�connfdû�����÷�����
    reactorģ�� one loop per thread
    poller+non-blocking IO
    */
    sockaddr_in addr;
    socklen_t len=sizeof addr;//���������Ҫ��ʼ����С
    bzero(&addr,sizeof addr);
    int connfd=::accept4(sockfd_,(sockaddr*)&addr,&len,SOCK_NONBLOCK|SOCK_CLOEXEC);//��accept���ñ�־λ
    if (connfd>=0)
    {
        peeraddr->setSockAddr(addr);//�ͻ���ͨ�������������
    }
    return connfd;//����������������
}

void Socket::shutdownWrite()//�ر�д��
{
    if (::shutdown(sockfd_,SHUT_WR)<0)
    {
        LOG_ERROR("shutdownWrite error");
    }
}

//socketЭ��ѡ��
void Socket::setTcpNoDelay(bool on)//Э�鼶��
{
    int optval=on?1:0;
    ::setsockopt(sockfd_,IPPROTO_TCP,TCP_NODELAY,&optval,sizeof optval);
}
void Socket::setReuseAddr(bool on)//socket����
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

