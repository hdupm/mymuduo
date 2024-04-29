#include "Buffer.h"
#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

/*
从fd上读取数据 Poller工作在LT模式 防止数据丢失
Buffer缓冲区是有大小的！但是从fd上读取数据的时候，却不知道tcp数据最终的大小
*/
ssize_t Buffer::readFd(int fd,int* saveErrno)
{
    char extrabuf[65536]={0};//栈上的内存空间 效率高 64k

    struct iovec vec[2];//iovec是结构体，内部存放缓冲区地址和大小
    //readv可以将从同一个fd中读出的数据，填充多个缓冲区 这和read不同

    //当readv从fd内读数据 先填充vec[0]的部分，不够再用vec[1]的，添加到缓冲区中，缓冲区大小刚刚好
    const size_t writable=writableBytes();//这是Buffer底层缓冲区剩余的可写空间大小
    vec[0].iov_base=begin()+writerIndex_;//开始写的起始地址
    vec[0].iov_len=writable;

    vec[1].iov_base=extrabuf;
    vec[1].iov_len=sizeof extrabuf;
    
    const int iovcnt=(writable<sizeof extrabuf)?2:1; //说明一次至少读64k数据
    const ssize_t n=::readv(fd,vec,iovcnt);
    if (n<0)
    {
        *saveErrno=errno;
    }
    else if (n<=writable) //读的小于writable 说明Buffer的可写缓冲区已经够存储读出来的数据了
    {
        writerIndex_+=n;
    }
    else//extrabuf里面也写入了数据 说明要将buffer扩容 添加进去
    {
        writerIndex_=buffer_.size();
        append(extrabuf,n-writable);//writerIndex_开始写 n-writable大小的数据
    }
    return n;
}

ssize_t Buffer::writeFd(int fd,int* saveErrno)
{
    ssize_t n=::write(fd,peek(),readableBytes());
    if (n<0)
    {
        *saveErrno=errno;
    }
    return n;
}