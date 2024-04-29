#include "Acceptor.h"
#include <sys/types.h>          
#include <sys/socket.h>
#include "Logger.h"
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include "InetAddress.h"

static int createNonblocking()//������IO SOCK_CLOEXEC�����ӽ����ܼ̳и���������fd ����Ĭ���ӽ��̹ر���Щfd
{
    int sockfd=::socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0);//���һ������ԭ����IPPROTO_TCP
    if (sockfd<0)
    {
        LOG_FATAL("%s:%s:%d listen socket create err:%d \n",__FILE__,__FUNCTION__,__LINE__,errno);
    }        
}

Acceptor::Acceptor(EventLoop *loop,const InetAddress &listenAddr,bool reuseport)//tcpserver�Ĳ���
    :loop_(loop)//���ʵ�ǰpoller
    ,acceptSocket_(createNonblocking())
    ,acceptChannel_(loop,acceptSocket_.fd())//��ǰloop��listenfd
    ,listenning_(false)
{
    acceptSocket_.setReuseAddr(true);//����tcpѡ��
    acceptSocket_.setReusePort(true);//�����׽���
    acceptSocket_.bindAddress(listenAddr);//bind�׽���
    //TcpServer::start() Acceptor.listen �����û����ӣ�Ҫִ��һ���ص�(connfd���=>channel=>subloop)
    //baseLoop=>acceptChannel_(listenfd)=>
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead,this));//���ûص����� ��channelע��Ķ��ص����� acceptchannelֻ���Ķ��¼� Ҳ����ֻ�������û�����
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();//��ע���¼�
    acceptChannel_.remove();//��channel�ӵ�ǰepoll�����
}


void Acceptor::listen()
{
    listenning_=true;//��������
    acceptSocket_.listen();//listen
    acceptChannel_.enableReading();//��channelע�ᵽ=>poller(mainloop��poller)���� poller������ ����е���readcallback

}

//listenfd���¼������ˣ����������û�������
//��acceptchannelע��Ķ��ص�����
void Acceptor::handleRead()
{
    InetAddress peerAddr;//peerAddr�ͻ�������
    int connfd=acceptSocket_.accept(&peerAddr);//����ͨ��connfd(cfd) 
    if (connfd>=0)//ִ�лص�����
    {
        if (newConnectionCallback_)//TcpServerԤ�ȸ�acceptor���õĻص����� ��������´���tcpconnection��
        {
            newConnectionCallback_(connfd,peerAddr);//��ѯ�ҵ�subloop�����ѣ��ַ���ǰ���¿ͻ��˵�Channel
        }
        else{
            ::close(connfd);//û�лص�����ֱ�ӹر�
        }
    }
    else
    {
        LOG_ERROR("%s:%s:%d accept err:%d \n",__FILE__,__FUNCTION__,__LINE__,errno);
        if (errno==EMFILE)//�ļ��������ľ�
        {
            LOG_ERROR("%s:%s:%d sockfd reached limit \n",__FILE__,__FUNCTION__,__LINE__);
        }
    }

}