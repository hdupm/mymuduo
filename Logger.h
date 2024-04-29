#pragma once
#include "noncopyable.h"
#include "Timestamp.h"
#include <string>
//LOG_INFO("%s %d",arg1,arg2) 如果宏有好几行代码 为了防止错误都是用的do while(0) 
//在宏的代码中要分割行可以在末尾加'\' 后面不能加空格
//__VA_ARGS__会在调用时替换成可变参数。
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
//一般调试信息不输出 这里用条件 MUDEBUG模式下可以输出
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
//定义日志的级别 INFO(打印重要流程信息) ERROR FATAL DEBUG(调试信息)默认关闭
enum Loglevel
{
    INFO,//普通信息
    ERROR,//错误模式
    FATAL,//core信息
    DEBUG,//调试信息
};

//输出一个日志类 单例 单例模式就是保证一个类只有一个实例，并提供一个访问它的全局访问点。
class Logger:noncopyable
{
public:
    //获取日志唯一的实例对象
    static Logger& instance();
    //设置日志级别
    void setLogLevel(int level);
    //写日志
    void log(std::string msg);
private:
    int logLevel_;//添加横线有助于区分函数局部变量和成员变量
    //Logger(){}
};