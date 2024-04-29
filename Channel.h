#pragma once
#include "noncopyable.h"
#include <functional>
#include "Timestamp.h"//ͷ�ļ������� ������ǰ������
#include <memory>
/*
Channel���Ϊͨ������װ��sockfd�������Ȥ��event����EPOLLIN��EPOLLOUT�¼�
������poller���صľ����¼�
�����EventLoop��Channel��Poller֮��Ĺ�ϵ ��=Reactorģ���϶�ӦDemultiplex
*/
class EventLoop;//����ǰ������ ��Ϊֻ��������Ͷ����ָ������ǰ����������

class Channel:noncopyable
{
public:
    using EventCallback=std::function<void()>; //�¼��ص�   ������c++��using����typedef ����ص�����EventCallback����
    using ReadEventCallback=std::function<void(Timestamp)>; //ֻ���¼��ص�
    Channel(EventLoop *Loop,int fd);
    ~Channel();

    //fd�õ�poller֪ͨ�Ժ󣬴����¼���
    /*
    ������epoll_wait()�󣬿��Ե�֪�¼�����������ЩChannel���ļ�����������������Щ�¼����¼���������Ȼ��Ҫ������ЩChannel��Ӧ�Ĵ�������
    Channel::HandleEvent����ÿ���������¼���Channel�����Լ����ܵ��¼���������ÿ��Channel������Լ��ļ�������ʵ�ʷ������¼���ͨ��Channel�е�revents_������֪���͸���Ȥ���¼���ͨ��Channel�е�events_������֪��
    ��ѡ�����read_callback_��/��write_callback_��/��close_callback_��/��error_callback_��

    */
    void handleEvent(Timestamp receiveTime);

    //���ûص��������� ChannelΪ����ļ�����������ĸ��¼����ͷ���ʱ�Ĵ�����
    void setReadCallback(ReadEventCallback cb) { readCallback_=std::move(cb);} //���ú�������ֵ���� ����βζ�����Ҫ ����ֵת����ֵ
    void setWriteCallback(EventCallback cb){writeCallback_=std::move(cb);}
    void setCloseCallback(EventCallback cb){closeCallback_=std::move(cb);}
    void setErrorCallback(EventCallback cb){ errorCallback_=std::move(cb);}

    //��ֹ��channel���ֶ�remove����channel����ָ��ص�����
    void tie(const std::shared_ptr<void>&);

    int fd() const {return fd_;}//fd��Poller�����Ķ��� epoll_ctl ������fd����Ͷ� ������Ҫ�޸�epoll
    int events()const{return events_;}
    //poll��ʹ�ü����¼� ���¼�������������һ��fd������ʲô�¼���ͨ��set_revents()����������reventsֵ��ͨ������������Խ�����ļ�������ʵ�ʷ������¼���װ�����Channel�С�
    void set_revents(int revt){revents_=revt;}
    

    //����fd��Ӧ���¼�״̬ ���������ChannelΪ����ļ�����������ĸ��¼����ͷ���ʱ�Ĵ�����
    /*
    �ⲿͨ���⼸����������֪Channel������ܵ��ļ�������������Щ�¼����͸���Ȥ����������ļ��������������Ȥ�¼�ע�ᵽ�¼���������IO��·����ģ�飩�ϡ�
    ��Щ�������涼��һ��update()˽�г�Ա���������update��ʵ�����Ͼ��ǵ�����epoll_ctl()��
    */
    void enableReading(){events_|=kReadEvent;update();}//�����¼���Ӧλ����1 update()��֪ͨepoll����epoll_ctl��fd����Ȥ�¼���ӵ�epoll����
    void disableReading(){events_&=~kReadEvent;update();}//ȥ�����¼� ��kReadEventȡ��
    void enableWriting(){events_|=kWriteEvent;update();}
    void disableWriting(){events_&=~kWriteEvent;update();}
    void disableAll(){events_=kNoneEvent;update();}

    //����fd��ǰ���¼�״̬
    bool isNoneEvent() const{return events_==kNoneEvent;}//�ж�fd�Ƿ�ע�����Ȥ�¼�
    bool isWriting() const{return events_& kWriteEvent;}
    bool isReading() const{return events_&kReadEvent;}
    int index(){return index_;}//��ҵ����� �Ͷ�·�ַ��������߼���ϵ����
    void set_index(int idx){index_=idx;}

    //one loop per thread
    //channel����һ��eventloop һ��eventloop��һ��poller һ��poller���Լ����ܶ�channel
    EventLoop* ownerLoop(){return loop_;}
    void remove();
private://�ų�Ա����

    void update();

    void handleEventWithGuard(Timestamp receiveTime);//�ܱ����Ĵ����¼�

    static const int kNoneEvent;//fd��û���¼�����Ȥ
    static const int kReadEvent;//fd�Զ��¼�����Ȥ
    static const int kWriteEvent;//fd��д�¼�����Ȥ

    EventLoop *loop_;//�¼�ѭ�� ��fd�����ĸ�EventLoop����
    const int fd_;//fd��poller�����Ķ��� ���channel�տ����ļ������� ��Ҫ��pollerע��
    int events_;//ע��fd����Ȥ���¼�����
    int revents_;//poller���صľ��巢�����¼����� ���¼�������������һ��fd������ʲô�¼���ͨ��Channel::set_revents()����������reventsֵ
    int index_;

    std::weak_ptr<void> tie_;//��������ָ��������� ��ֹ����remove��channel ʹ�ÿ��̵߳Ķ�������״̬�ļ��� void��ʾ�κζ��󶼿��Խ���
    bool tied_;//�󶨵�ǰ����

    //��Ϊchannelͨ�������ܹ���֪fd���շ����ľ�����¼�revents��������������þ����¼��Ļص�����
    ReadEventCallback readCallback_;//�ĸ���������
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};