#include "EPollPoller.h"
#include "Logger.h"
#include <errno.h>
#include <unistd.h>
#include "Channel.h"
#include <string.h>

//表示channel还未添加到poller中 channel的参数index_初始化为-1
const int kNew=-1;
//表示channel已添加到poller中
const int kAdded=1;
//channel从poller中删除
const int kDeleted=2;

EPollPoller::EPollPoller(EventLoop *loop) //常量表示当前channel和epoll的状态
    :Poller(loop)//首先调用基类构造函数来初始化从基类继承的成员   ::用于指定全局命名空间
    ,epollfd_(::epoll_create1(EPOLL_CLOEXEC)) //EPOLL_CLOEXEC是flag
    ,events_(kInitEventListSize) //vector<epoll_event> 用vector用作线性存储
{
    if (epollfd_<0)
    {
        LOG_FATAL("epoll_create error:%d\n",errno);
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

//调用poller
/*
当外部调用poll方法的时候，该方法底层其实是通过epoll_wait获取这个事件监听器上发生事件的fd及其对应发生的事件，我们知道每个fd都是由一个Channel封装的，通过哈希表channels_可以根据fd找到封装这个fd的Channel。
将事件监听器监听到该fd发生的事件写进这个Channel中的revents成员变量中。然后把这个Channel装进activeChannels中（它是一个vector<Channel*>）。这样，当外界调用完poll之后就能拿到事件监听器的监听结果（activeChannels_）
*/
Timestamp EPollPoller::poll(int timeoutMs,ChannelList *activateChannels)//epoll_wait 参数是超时时间 对传入的list进行监听哪些channel发生事件 timeoutMs是阻塞时长 到达一段时间后必须返回 因为是非阻塞
{
    //实际上应该用LOG_DEBUG输出日志更为合理
    LOG_INFO("func=%s => fd total count:%lu \n",__FUNCTION__,channels_.size());

    ///static_cast<int>是类型强转 将unsigned转为signed传入
    int numEvents=::epoll_wait(epollfd_,&*events_.begin(),static_cast<int> (events_.size()),timeoutMs);//第二个参数本来是存放发生event事件的数组 为了方便扩容用vector events_begin()是数组首元素地址 解引用是首元素的值
    int saveErrno=errno;//将错误号用局部变量存放
    Timestamp now(Timestamp::now());

    if (numEvents>0)//发生事件个数
    {
        LOG_INFO("%d events happened \n",numEvents);
        fillActivateChannels(numEvents,activateChannels);
        if (numEvents==events_.size())//需要扩容 events_是定义的vector数组
        {
            events_.resize(events_.size()*2);
        }
    }
    else if (numEvents==0)//由于超时
    {
        LOG_DEBUG("%s timeout!\n",__FUNCTION__);
    }
    else{
        //外部中断
        if (saveErrno!=EINTR)
        {
            errno=saveErrno;//防止其他错误冲突
            LOG_ERROR("EPollPoller::poll() err!");
        }
    }
    return now;//返回具体时间点
}

//channel update remove =>EventLoop updateChannel removeChannel=>Poller
/*
                    EventLoop=> poller.poll
        ChannelList             Poller
    存放所有channel(fd)  >=    ChannelMap <fd,channel*> 向poller注册后的
    无论是否向poller注册过
*/
void EPollPoller::updateChannel(Channel *channel) //epoll_ctl
{
    const int index=channel->index();
    LOG_INFO("func=%s => fd=%d events=%d index=%d \n",__FUNCTION__,channel->fd(),channel->events(),index);//函数名是编译器默认提供的
    if (index==kNew||index==kDeleted)
    {
        if (index==kNew)//说明没向poller添加过
        {
            int fd=channel->fd();
            channels_[fd]=channel;
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD,channel);
    }
    else //channel已经在poller上注册过了
    {
        int fd=channel->fd();
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL,channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD,channel);
        }
    }

}

//从poller中删除channel
void EPollPoller::removeChannel(Channel *channel) //epoll_ctl 可能在channellist中，但要从epoll的channelmap中remove掉
{
    int fd=channel->fd();    
    channels_.erase(fd);
    LOG_INFO("func=%s => fd=%d\n",__FUNCTION__,fd);
    int index=channel->index();
    if (index==kAdded)
    {
        update(EPOLL_CTL_DEL,channel);
    }
    channel->set_index(kNew);

}

void EPollPoller::fillActivateChannels(int numEvents,ChannelList *activateChannels) const//填写活跃channel
{
    for (int i=0;i<numEvents;i++)
    {
        Channel *channel=static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activateChannels->push_back(channel);//eventloop就拿到了它的poller给它返回的所有发生时间的channel列表了
    }
}

//更新channel通道 epoll_ctl add/mod/del    
void EPollPoller::update(int operation,Channel* channel)
{
    epoll_event event;
    bzero(&event,sizeof event);//清空为0 或者用memset也可以，但需要传第二个参数作为重置数字
    
    int fd=channel->fd();

    event.events=channel->events();//返回fd感兴趣事件
    event.data.fd=fd;
    event.data.ptr=channel;//data是epoll_data_t类型，void *ptr是针对fd，这里用ptr指向fd对应的channel 还可以携带fd参数
    
    if (::epoll_ctl(epollfd_,operation,fd,&event)<0)
    {
        if (operation==EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_stl del error:%d\n",errno);//如果是删除失败，事情不算严重 打印错误信息即可
        }
        else
        {
            LOG_FATAL("epoll_stl add/mod error:%d\n",errno);//修改/创建失败导致FATAL 自动exit
        }
    }
}