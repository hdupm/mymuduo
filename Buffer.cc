#include "Buffer.h"
#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

/*
��fd�϶�ȡ���� Poller������LTģʽ ��ֹ���ݶ�ʧ
Buffer���������д�С�ģ����Ǵ�fd�϶�ȡ���ݵ�ʱ��ȴ��֪��tcp�������յĴ�С
*/
ssize_t Buffer::readFd(int fd,int* saveErrno)
{
    char extrabuf[65536]={0};//ջ�ϵ��ڴ�ռ� Ч�ʸ� 64k

    struct iovec vec[2];//iovec�ǽṹ�壬�ڲ���Ż�������ַ�ʹ�С
    //readv���Խ���ͬһ��fd�ж��������ݣ������������ ���read��ͬ

    //��readv��fd�ڶ����� �����vec[0]�Ĳ��֣���������vec[1]�ģ���ӵ��������У���������С�ոպ�
    const size_t writable=writableBytes();//����Buffer�ײ㻺����ʣ��Ŀ�д�ռ��С
    vec[0].iov_base=begin()+writerIndex_;//��ʼд����ʼ��ַ
    vec[0].iov_len=writable;

    vec[1].iov_base=extrabuf;
    vec[1].iov_len=sizeof extrabuf;
    
    const int iovcnt=(writable<sizeof extrabuf)?2:1; //˵��һ�����ٶ�64k����
    const ssize_t n=::readv(fd,vec,iovcnt);
    if (n<0)
    {
        *saveErrno=errno;
    }
    else if (n<=writable) //����С��writable ˵��Buffer�Ŀ�д�������Ѿ����洢��������������
    {
        writerIndex_+=n;
    }
    else//extrabuf����Ҳд�������� ˵��Ҫ��buffer���� ��ӽ�ȥ
    {
        writerIndex_=buffer_.size();
        append(extrabuf,n-writable);//writerIndex_��ʼд n-writable��С������
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