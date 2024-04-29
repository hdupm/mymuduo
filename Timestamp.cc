#include "Timestamp.h"
#include <time.h>
#include <iostream>

Timestamp::Timestamp():microSecondsSinceEpoch_(0){}
Timestamp::Timestamp(int64_t microSecondsSinceEpoch_)
    :microSecondsSinceEpoch_(microSecondsSinceEpoch_)
    {}
Timestamp Timestamp::now()//通过调用now获取一个表示长整型的时间
{
    return Timestamp(time(NULL));
}
std::string Timestamp::toString() const
{
    char buf[128]={0};
    tm *tm_time=localtime(&microSecondsSinceEpoch_);
    snprintf(buf,128,"%4d/%02d/%02d %02d:%02d:%02d",//04表示4位 分别代表年月日 时分秒
    tm_time->tm_year+1900,
    tm_time->tm_mon+1,
    tm_time->tm_mday,
    tm_time->tm_hour,
    tm_time->tm_min,
    tm_time->tm_sec); 
    return buf;
}

//测试代码
// int main()
// {
//     std::cout<<Timestamp::now().toString()<<std::endl;
//     return 0;
// }

/*
在这个代码中，Timestamp::now() 方法通过调用 time(NULL) 函数获取当前时间的时间戳，
然后将其转换为 Timestamp 类型并返回。Timestamp::toString() 方法将时间戳转换为年月日时分秒的字符串表示形式，并返回该字符串。
*/