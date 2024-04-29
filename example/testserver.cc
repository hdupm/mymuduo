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
        //注册回调函数
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection,this,std::placeholders::_1)
        );
        server_.setMessageCallback(
            std::bind(&EchoServer::onMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3)
        );
        //设置合适的loop线程数量 加上主线程共4个
        server_.setThreadNum(3);
    }
    void start()
    {
        server_.start();
    }
private:
    //连接建立或者断开的回调 业务处理
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

    //可读写事件回调 回声服务器逻辑 业务处理
    void onMessage(const TcpConnectionPtr &conn,
                Buffer *buf,
                Timestamp time)
    {
        std::string msg=buf->retrieveAllAsString();//用string更通用，无论用户发的json buffer都能使用 拿到原始字符串
        conn->send(msg);
        conn->shutdown();//完成发送后关闭连接，写端关闭 EPOLLHUP=>closeCallback 删除连接   这里一个请求一个连接 后期可以优化
    }

    EventLoop *loop_;//mainloop
    TcpServer server_;

};


int main()
{
    EventLoop loop;//构建mainLoop
    InetAddress addr(8000);//定义本地地址端口8000
    EchoServer server(&loop,addr,"EchoServer-01");//Acceptor non-blocking listenfd create bind
    server.start();//listen loopthread listenfd => acceptChannel => mainLoop => 
    loop.loop();//启动mainLoop的底层Poller 开启baseloop的loop 开始监听并处理事件

    return 0;
}