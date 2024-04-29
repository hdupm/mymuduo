#include "InetAddress.h"
#include <iostream>
#include <cstring>
#include <arpa/inet.h>

InetAddress::InetAddress(uint16_t port,std::string ip)
{
    bzero(&addr_,sizeof addr_);//将字节序清零
    addr_.sin_family=AF_INET;
    addr_.sin_port=htons(port); //本地字节序转成网络字节序
    addr_.sin_addr.s_addr=inet_addr(ip.c_str());//inet_addr将字符串转成整数表示 并转成网络字节序   
}
std::string InetAddress::toIp() const
{
    //addr_
    char buf[64]={0};
    ::inet_ntop(AF_INET,&addr_.sin_addr,buf,sizeof buf);
    return buf;
}
std::string InetAddress::toIpPort() const
{
    //ip:port
    char buf[64]={0};
    ::inet_ntop(AF_INET,&addr_.sin_addr,buf,sizeof buf);
    size_t end=strlen(buf);//ip的有效长度
    uint16_t port=ntohs(addr_.sin_port);
    sprintf(buf+end,":%u",port);//从刚填的ip地址往下填
    return buf;
}
uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);
}

//测试 
// int main()
// {
//     InetAddress addr(8080);//传入指定端口
//     std::cout<<addr.toIpPort()<<std::endl;
//     return 0;
// }