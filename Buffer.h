#pragma once
#include <vector>
#include <algorithm>
#include <string>

/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |     可读缓冲区    |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode
//解决粘包问题，可以在开头prependable写入解析包长度，后面就是数据。向缓冲区写就从writable写，应用程序读从readable读

//网络库底层的缓冲器类型定义
class Buffer
{
public:
    static const size_t kCheapPrepend=8;//数据包长度
    static const size_t kInitialSize=1024;//缓冲区初始大小

    explicit Buffer(size_t initialSize=kInitialSize)
        :buffer_(kCheapPrepend+initialSize)//底层默认开辟长度
        ,readerIndex_(kCheapPrepend)
        ,writerIndex_(kCheapPrepend)
    {}

    size_t readableBytes() const
    {
        return writerIndex_-readerIndex_;
    }

    size_t writableBytes() const
    {
        return buffer_.size()-writerIndex_;//可写空间大小

    }

    size_t prependableBytes() const //如果reader被独奏一部分，那么prependableBytes=8字节+被读走的字节
    {
        return readerIndex_;
    }

    //返回缓冲区中可读数据的起始地址
    const char* peek() const
    {
        return begin()+readerIndex_;//返回可读数组+readerIndex
    }

    //onMessage string<-Buffer
    void retrieve(size_t len)
    {
        if (len<readableBytes())//有部分数据未读 应用只读取了可读缓冲区数据的一部分，就是len，还剩下readerIndex_+=len->writerIndex_未读
        {
            readerIndex_+=len;
        }
        else //len==readableBytes() 应用全部读取
        {
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_=writerIndex_=kCheapPrepend;//读取结束后复位操作
    }

    //把onMessage函数上报的Buffer数据，转成string类型的数据返回给应用
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());//应用可读取的数据的长度
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(),len);//(可读数据起始地址，可读长度)
        retrieve(len);//上面一句把缓冲区中可读的数据读取出来，这里肯定要对缓冲区进行复位操作
        return result;
    }

    //buffer_.size-writeIndex_  len
    void ensureWriteableBytes(size_t len)
    {
        if (writableBytes()<len)
        {
            makeSpace(len);//扩容函数
        }
    }

    //将[data,data+len]内存上的数据，添加到可写缓冲区中
    void append(const char *data,size_t len)
    {
        ensureWriteableBytes(len);
        std::copy(data,data+len,beginWrite());
        writerIndex_+=len;
    }

    char* beginWrite()//返回可写起始地址
    {
        return begin()+writerIndex_;
    }

    const char* beginWrite() const
    {
        return begin()+writerIndex_;
    }

    //从fd上读取数据
    ssize_t readFd(int fd,int* saveErrno);
    //通过fd发送数据
    ssize_t writeFd(int fd,int* saveErrno);

private:
    char* begin()
    {
        //it.operator*()
        return &*buffer_.begin();//迭代器解引号 获取迭代器首位元素并取地址 即vector底层数组首元素的地址，也就是数组起始地址
    }

    const char* begin() const
    {
        return &*buffer_.begin();
    }

    void makeSpace(size_t len)
    {
        /*
        kCheapPrepend | reader | writer     reader已经被使用，可使用缓冲区只有 kCheapPrepend 8字节+reader被读走后空闲的字节+writer区域字节数
        kCheapPrepend |        len      |需要缓冲区长度
        */
       if (writableBytes()+prependableBytes()<len+kCheapPrepend)//说明需要resize扩容
       {
            buffer_.resize(writerIndex_+len);
       }
       else //仍可以调整  如果超出 将reader未读部分移到前面prepend区域后边 空后面区域给writer 
       {
            size_t readable=readableBytes();
            std::copy(begin()+readerIndex_,begin()+writerIndex_,begin()+kCheapPrepend);
            readerIndex_=kCheapPrepend;
            writerIndex_=readerIndex_+readable;
       }
    }    

    std::vector<char> buffer_;//用vector扩容方便
    size_t readerIndex_;
    size_t writerIndex_;


};