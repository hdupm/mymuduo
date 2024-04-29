#include <mymuduo/TcpServer.h>
#include <mymuduo/Logger.h>
#include <functional>
#include <string>


class EchoServer
{
public:
    EchoServer(EventLoop *loop,const InetAddress &addr,const std::string &name)
        :server_(loop,addr,name)
        ,loop_(loop)
    {
        //ע��ص�����
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection,this,std::placeholders::_1)
        );
        server_.setMessageCallback(
            std::bind(&EchoServer::onMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3)
        );
        //���ú��ʵ�loop�߳����� �������̹߳�4��
        server_.setThreadNum(3);
    }
    void start()
    {
        server_.start();
    }
private:
    //���ӽ������߶Ͽ��Ļص� ҵ����
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            LOG_INFO("Connection UP:%s",conn->peerAddress().toIpPort().c_str());
        }
        else
        {
            LOG_INFO("Connection DOWN:%s",conn->peerAddress().toIpPort().c_str());
        }
    }

    //�ɶ�д�¼��ص� �����������߼� ҵ����
    void onMessage(const TcpConnectionPtr &conn,
                Buffer *buf,
                Timestamp time)
    {
        std::string msg=buf->retrieveAllAsString();//��string��ͨ�ã������û�����json buffer����ʹ�� �õ�ԭʼ�ַ���
        conn->send(msg);
        conn->shutdown();//��ɷ��ͺ�ر����ӣ�д�˹ر� EPOLLHUP=>closeCallback ɾ������   ����һ������һ������ ���ڿ����Ż�
    }

    EventLoop *loop_;//mainloop
    TcpServer server_;

};


int main()
{
    EventLoop loop;//����mainLoop
    InetAddress addr(8000);//���屾�ص�ַ�˿�8000
    EchoServer server(&loop,addr,"EchoServer-01");//Acceptor non-blocking listenfd create bind
    server.start();//listen loopthread listenfd => acceptChannel => mainLoop => 
    loop.loop();//����mainLoop�ĵײ�Poller ����baseloop��loop ��ʼ�����������¼�

    return 0;
}