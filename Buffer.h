#pragma once
#include <vector>
#include <algorithm>
#include <string>

/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |     �ɶ�������    |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode
//���ճ�����⣬�����ڿ�ͷprependableд����������ȣ�����������ݡ��򻺳���д�ʹ�writableд��Ӧ�ó������readable��

//�����ײ�Ļ��������Ͷ���
class Buffer
{
public:
    static const size_t kCheapPrepend=8;//���ݰ�����
    static const size_t kInitialSize=1024;//��������ʼ��С

    explicit Buffer(size_t initialSize=kInitialSize)
        :buffer_(kCheapPrepend+initialSize)//�ײ�Ĭ�Ͽ��ٳ���
        ,readerIndex_(kCheapPrepend)
        ,writerIndex_(kCheapPrepend)
    {}

    size_t readableBytes() const
    {
        return writerIndex_-readerIndex_;
    }

    size_t writableBytes() const
    {
        return buffer_.size()-writerIndex_;//��д�ռ��С

    }

    size_t prependableBytes() const //���reader������һ���֣���ôprependableBytes=8�ֽ�+�����ߵ��ֽ�
    {
        return readerIndex_;
    }

    //���ػ������пɶ����ݵ���ʼ��ַ
    const char* peek() const
    {
        return begin()+readerIndex_;//���ؿɶ�����+readerIndex
    }

    //onMessage string<-Buffer
    void retrieve(size_t len)
    {
        if (len<readableBytes())//�в�������δ�� Ӧ��ֻ��ȡ�˿ɶ����������ݵ�һ���֣�����len����ʣ��readerIndex_+=len->writerIndex_δ��
        {
            readerIndex_+=len;
        }
        else //len==readableBytes() Ӧ��ȫ����ȡ
        {
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_=writerIndex_=kCheapPrepend;//��ȡ������λ����
    }

    //��onMessage�����ϱ���Buffer���ݣ�ת��string���͵����ݷ��ظ�Ӧ��
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());//Ӧ�ÿɶ�ȡ�����ݵĳ���
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(),len);//(�ɶ�������ʼ��ַ���ɶ�����)
        retrieve(len);//����һ��ѻ������пɶ������ݶ�ȡ����������϶�Ҫ�Ի��������и�λ����
        return result;
    }

    //buffer_.size-writeIndex_  len
    void ensureWriteableBytes(size_t len)
    {
        if (writableBytes()<len)
        {
            makeSpace(len);//���ݺ���
        }
    }

    //��[data,data+len]�ڴ��ϵ����ݣ���ӵ���д��������
    void append(const char *data,size_t len)
    {
        ensureWriteableBytes(len);
        std::copy(data,data+len,beginWrite());
        writerIndex_+=len;
    }

    char* beginWrite()//���ؿ�д��ʼ��ַ
    {
        return begin()+writerIndex_;
    }

    const char* beginWrite() const
    {
        return begin()+writerIndex_;
    }

    //��fd�϶�ȡ����
    ssize_t readFd(int fd,int* saveErrno);
    //ͨ��fd��������
    ssize_t writeFd(int fd,int* saveErrno);

private:
    char* begin()
    {
        //it.operator*()
        return &*buffer_.begin();//������������ ��ȡ��������λԪ�ز�ȡ��ַ ��vector�ײ�������Ԫ�صĵ�ַ��Ҳ����������ʼ��ַ
    }

    const char* begin() const
    {
        return &*buffer_.begin();
    }

    void makeSpace(size_t len)
    {
        /*
        kCheapPrepend | reader | writer     reader�Ѿ���ʹ�ã���ʹ�û�����ֻ�� kCheapPrepend 8�ֽ�+reader�����ߺ���е��ֽ�+writer�����ֽ���
        kCheapPrepend |        len      |��Ҫ����������
        */
       if (writableBytes()+prependableBytes()<len+kCheapPrepend)//˵����Ҫresize����
       {
            buffer_.resize(writerIndex_+len);
       }
       else //�Կ��Ե���  ������� ��readerδ�������Ƶ�ǰ��prepend������ �պ��������writer 
       {
            size_t readable=readableBytes();
            std::copy(begin()+readerIndex_,begin()+writerIndex_,begin()+kCheapPrepend);
            readerIndex_=kCheapPrepend;
            writerIndex_=readerIndex_+readable;
       }
    }    

    std::vector<char> buffer_;//��vector���ݷ���
    size_t readerIndex_;
    size_t writerIndex_;


};