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

static EventLoop* CheckLoopNotNull(EventLoop *loop)//��̬��ֹ������ͻ
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
            ,highWaterMark_(64*1024*1024)//64M �����������ò��� ����ֹͣ����
{
    //��channel������Ӧ�Ļص�������poller֪ͨchannel����Ȥ���¼������ˣ�channel��ص���Ӧ�Ĳ�������
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
    LOG_INFO("TcpConnection::Dtor[%s] at fd=%d state=%d \n",name_.c_str(),channel_->fd(),(int)state_);//ö��������Ҫ����ת������
}

void TcpConnection::send(const std::string &buf)
{
    if (state_==kConnected)
    {
        if (loop_->isInLoopThread())//��ǰloop�ڶ�Ӧ�߳���
        {
            sendInLoop(buf.c_str(),buf.size());            
        }
        else//ִ�лص�
        {
            loop_->runInLoop(std::bind(&TcpConnection::sendInLoop,this,buf.c_str(),buf.size()));
        }
    }
}

/*
�������� Ӧ��д�Ŀ죬���ں˷�������������Ҫ�Ѵ���������д�뻺����������������ˮλ�ص�
*/
void TcpConnection::sendInLoop(const void* data, size_t len)
{
    ssize_t nwrote=0;
    size_t remaining=len;//û�����������
    bool faultError=false;

    //֮ǰ���ù���connection��shutdown�������ٽ��з�����
    if (state_==kDisconnected)
    {
        LOG_ERROR("disconnected,give up writing!");
        return;
    }

    //��ʾchannel_��һ�ο�ʼд���ݣ����һ�����û�д���������     
    if (!channel_->isWriting() && outputBuffer_.readableBytes()==0)
    {
        nwrote=::write(channel_->fd(),data,len);//ʵ��д�����
        if (nwrote>=0)
        {
            remaining=len-nwrote;//��Ҫд��-ʵ��д��
            if (remaining==0 && writeCompleteCallback_)
            {
                //��Ȼ����������ȫ��������ɣ��Ͳ��ø�channel����epollout�¼���
                loop_->queueInLoop(
                    std::bind(writeCompleteCallback_,shared_from_this())
                );
            }
            
        }
        else //nwrote<0
        {
            nwrote=0;
            if (errno!=EWOULDBLOCK)//�Ƿ�����
            {
                LOG_ERROR("TcpConnection::sendInLoop");
                if (errno==EPIPE||errno==ECONNRESET)//SIGPIPE RESET ���յ��Զ����� ���Ӵ���
                {
                    faultError=true;
                }

            }
        }
    }

    //˵����ǰ��һ��write��û�а�����ȫ�����ͳ�ȥ��ʣ���������Ҫ���浽���������У�Ȼ���channelע��epollout�¼�
    //(��Ϊ��LTģʽ)poller����tcp�ķ��ͻ������пռ䣬��֪ͨ��Ӧ��sock-channel������channel->writecallback_�ص�����
    //Ҳ���ǵ���TcpConnection::handleWrite�������ѷ��ͻ�����������ȫ���������
    //�������ϴ�������޷�һ���Խ���������д�뵽channel��
    if (!faultError && remaining>0)
    {
        //Ŀǰ���ͻ�����ʣ��Ĵ��������ݵĳ���
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
            channel_->enableWriting();//����һ��Ҫע��channel��д�¼�������poller�����channel֪ͨepollout
        }
    }
}

//���ӽ��� TcpConnection��channel������Ӧ�Ļص�
void TcpConnection::connectEstablished() // should be called only once
{
    setState(kConnected);
    channel_->tie(shared_from_this());//��¼TcpConnection״̬
    channel_->enableReading();//��pollerע��channel��epollin�¼�
    //�����ӽ��� ִ�лص� 
    connectionCallback_(shared_from_this());
}   

// TcpServer::removeConnection���ã���TcpServer����ǰconnection����������б����Ƴ������
// �ڲ����壬������ֱ�ӵ��ã�����Eventloop��functors�б���
// loop_->queueInLoop(boost::bind(&TcpConnection::connectDestroyed, conn));
// ��������
void TcpConnection::connectDestroyed() // should be called only once
{
    if (state_==kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll();//��channel���и���Ȥ�¼���poller��del��
        connectionCallback_(shared_from_this());
    }
    channel_->remove();//��channel��poller��ɾ����
}

//�ر����� �����û����ã�����ʱ�ײ��п�����д�¼�û����
//muduo��ÿһ��loop��Ҫ�������߳���ִ��
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

//TcpConnection::shutdown()->shutdownInLoop->socket�ر�д��->epoll֪ͨchannel�ر��¼�->�ص�TcpConnection::handleClose()����
void TcpConnection::shutdownInLoop()
{
    if (!channel_->isWriting())//channel_û��д�¼� ˵��outputBuffer�е������Ѿ�ȫ���������
    {
        socket_->shutdownWrite();//�ر�д�� ����epoll�е�EPOLLHUP(����ע��)
    }
}

void TcpConnection::handleRead(Timestamp receiveTime)// ��ͨ�����ֿɶ��¼�����ص�
{
    int savedErrno=0;
    ssize_t n=inputBuffer_.readFd(channel_->fd(),&savedErrno);
    if (n>0)
    {
        //�ѽ������ӵ��û����пɶ��¼������ˣ������û�����Ļص�����onMessage
        messageCallback_(shared_from_this(),&inputBuffer_,receiveTime);  //shared_from_this�ǻ�ȡ��ǰtcpconnection���������ָ��
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
    if (channel_->isWriting()) //�ж�channel�Ƿ��д
    {
        int savedErrno=0;
        ssize_t n=outputBuffer_.writeFd(channel_->fd(),&savedErrno);
        if (n>0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes()==0)//˵���������
            {
                channel_->disableWriting();
                if (writeCompleteCallback_)
                {
                    //����loop_��Ӧ��thread�̣߳�ִ�лص�
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_,shared_from_this())
                    );
                }
                if (state_==kDisconnecting)
                {
                    shutdownInLoop();//�ڵ�ǰloop�н�TcpConnectionɾ����
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
    else//channel��д�¼�������Ȥ
    {
        LOG_ERROR("TcpConnection fd=%d is down,no more writing \n",channel_->fd());
    }
}

//poller=>channel::closeCallback=>TC��Ԥ�õĻص�����TcpConnection::handleClose
void TcpConnection::handleClose()                    // ���ӹر�ʱ�ص�
{
    LOG_INFO("TcpConnection::handleClose fd=%d state=%d \n",channel_->fd(),(int)state_);
    setState(kDisconnected);
    channel_->disableAll();
    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr);//ִ�����ӹرյĻص�
    closeCallback_(connPtr);//�ر����ӵĻص� ִ�е���TcpServer::removeConnection�ص�����
}

void TcpConnection::handleError()                    // �¼��п��ܻ��д���
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