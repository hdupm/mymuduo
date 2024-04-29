#pragma once
#include "noncopyable.h"
#include "Timestamp.h"
#include <string>
//LOG_INFO("%s %d",arg1,arg2) ������кü��д��� Ϊ�˷�ֹ�������õ�do while(0) 
//�ں�Ĵ�����Ҫ�ָ��п�����ĩβ��'\' ���治�ܼӿո�
//__VA_ARGS__���ڵ���ʱ�滻�ɿɱ������
#define LOG_INFO(LogmsfFormat,...)\
    do \
    { \
        Logger &logger=Logger::instance();\
        logger.setLogLevel(INFO);\
        char buf[1024]={0};\
        snprintf(buf,1024,LogmsfFormat,##__VA_ARGS__);\
        logger.log(buf);\
    }while(0)

#define LOG_ERROR(LogmsfFormat,...)\
    do \
    { \
        Logger &logger=Logger::instance();\
        logger.setLogLevel(ERROR);\
        char buf[1024]={0};\
        snprintf(buf,1024,LogmsfFormat,##__VA_ARGS__);\
        logger.log(buf);\
    }while(0)

#define LOG_FATAL(LogmsfFormat,...)\
    do \
    { \
        Logger &logger=Logger::instance();\
        logger.setLogLevel(FATAL);\
        char buf[1024]={0};\
        snprintf(buf,1024,LogmsfFormat,##__VA_ARGS__);\
        logger.log(buf);\
        exit(-1);\
    }while(0)
//һ�������Ϣ����� ���������� MUDEBUGģʽ�¿������
#ifdef MUDEBUG 
#define LOG_DEBUG(LogmsfFormat,...)\
    do \
    { \
        Logger &logger=Logger::instance();\
        logger.setLogLevel(DEBUG);\
        char buf[1024]={0};\
        snprintf(buf,1024,LogmsfFormat,##__VA_ARGS__);\
        logger.log(buf);\
    }while(0)
#else  
    #define LOG_DEBUG(LogmsfFormat,...)
#endif
//������־�ļ��� INFO(��ӡ��Ҫ������Ϣ) ERROR FATAL DEBUG(������Ϣ)Ĭ�Ϲر�
enum Loglevel
{
    INFO,//��ͨ��Ϣ
    ERROR,//����ģʽ
    FATAL,//core��Ϣ
    DEBUG,//������Ϣ
};

//���һ����־�� ���� ����ģʽ���Ǳ�֤һ����ֻ��һ��ʵ�������ṩһ����������ȫ�ַ��ʵ㡣
class Logger:noncopyable
{
public:
    //��ȡ��־Ψһ��ʵ������
    static Logger& instance();
    //������־����
    void setLogLevel(int level);
    //д��־
    void log(std::string msg);
private:
    int logLevel_;//��Ӻ������������ֺ����ֲ������ͳ�Ա����
    //Logger(){}
};