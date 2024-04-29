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
                Option option)//�����ʱ����Ĭ��ֵ����Ϊ������ʱ���Ѿ�����
                :loop_(loop)//�������û�����loop
                ,ipPort_(listenAddr.toIpPort())//�û�����ip�Ͷ˿�
                ,name_(nameArg)
                ,acceptor_(new Acceptor(loop,listenAddr,option==kReusePort)) //��acceptor��mainloop�� �����൱�ڴ���socket����
                ,threadPool_(new EventLoopThreadPool(loop,name_))
                ,connectionCallback_()//�û����û��ע��ص���������ôִ��Ĭ�ϵ� �ûص�����������ص�ַ���Զ˵�ַ������״̬
                ,messageCallback_()//��buffer�еĶ�������д��������
                ,nextConnId_(1)//��һ������������IDλ1
                ,started_(0)
{
    //�������û�����ʱ����ִ��TcpServer::newConnection�ص�(����acceptor��acceptor::handleread()�����ʹ�øûص�����)
    //���� ��������ϼ��û����õĻص� ��Ϊ����ص������Ĵ������cb
    //���������acceptor��channel�Ļص�����setReadCallback��Ҳ������������ӵĶ��¼������õĻص�����
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection,this,
        std::placeholders::_1,std::placeholders::_2)); //������һ���Ǵ����Լ�����Ϊacceptor�Ļص�������������������������ռλ��
}

TcpServer::~TcpServer()
{
    for (auto &item:connections_) //�����ӵ�����ָ���ͷ� ������ǿ����ָ�������Դ �����þֲ�������� �����޷��ͷ� ���ɾ������
    {
        TcpConnectionPtr conn(item.second);//����ֲ���shared_ptr����ָ����� �������ſ����Զ��ͷ�new������TcpConnection������Դ��   
        item.second.reset();//�����ӵ�����ָ���ͷ�

        //��������
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed,conn)
        );
    }
}

//���õײ�subloop�ĸ���
void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

//��������������
void TcpServer::start()
{
    if (started_++==0) //��ֹһ��TcpServer����start���
    {
        threadPool_->start(threadInitCallback_);//�����ײ��̳߳�
        loop_->runInLoop(std::bind(&Acceptor::listen,acceptor_.get()));//����listen Acceptor::listen�ǽ�acceptchannelע������baseloop��
    }
}

//��һ���µĿͻ��˵����ӣ�acceptor��ִ������ص����� ����mainloop�ﴦ����˲��漰�̰߳�ȫ
//�����������֡��������ӡ������Ӱ󶨻ص�
void TcpServer::newConnection(int sockfd,const InetAddress &peerAddr)
{
    //��ѯ�㷨��ѡ��һ��sunLoop��������channel
    EventLoop *ioLoop=threadPool_->getNextLoop();
    char buf[64]={0};
    snprintf(buf,sizeof buf,"-%s#%d",ipPort_.c_str(),nextConnId_);//��ʾ��������
    ++nextConnId_;
    std::string connName=name_+buf;//��������

    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s \n",
        name_.c_str(),connName.c_str(),peerAddr.toIpPort().c_str());

    //ͨ��sockfd��ȡ��󶨵ı�����ip��ַ�Ͷ˿���Ϣ
    sockaddr_in local;
    ::bzero(&local,sizeof local);
    socklen_t addrlen=sizeof local;
    if (::getsockname(sockfd,(sockaddr*)&local,&addrlen)<0)
    {
        LOG_ERROR("sockets::getLocalAddr");
    }
    InetAddress localAddr(local);

    //�������ӳɹ���sockfd������TcpConnection���Ӷ���
    TcpConnectionPtr conn(new TcpConnection(
                        ioLoop,
                        connName,
                        sockfd, //ͨ����Щ��Ϣ�����ײ�Socket Channel����
                        localAddr,
                        peerAddr));
    connections_[connName]=conn;
    //����Ļص������û����ø�TcpServer=>TcpConnection=>Channel=>ע��Poller=>notify channel���ûص�
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    //��������ιر����ӵĻص� conn->shutDown()
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection,this,std::placeholders::_1)
    );

    //ֱ�ӵ��� ע��TcpConnection�������߳��еģ�������������߳�ִ��
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
    connections_.erase(conn->name());//��map����ɾ�� conn��������Ϊmap��
    EventLoop *ioLoop=conn->getLoop();
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed,conn)
    );

}