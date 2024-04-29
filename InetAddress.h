#pragma once
#include <netinet/in.h>
#include <string>

//封装socket地址类型(只封装了ipv4) c++中struct可省略，因为struct定义的东西可以看作类 但c中要写
class InetAddress
{
public:
    explicit InetAddress(uint16_t port=0,std::string ip="127.0.0.1");//如果不传ip默认是回环地址127.0.0.1 端口号也可以指定 端口号 0 是一种由系统指定动态生成的端口。
    explicit InetAddress(const sockaddr_in &addr)
    :addr_(addr)
    {}
    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;
    const sockaddr_in* getSockAddr() const//获取成员变量
    {
        return &addr_;
    }
    void setSockAddr(const sockaddr_in &addr){addr_=addr;}
private:
    sockaddr_in addr_;

};