#pragma once
#include <iostream>
//时间类
class Timestamp
{
public:
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpoch_);//带参数的构造函数加上explicit 指明不能是隐式转换对象
    static Timestamp now();//获取当前时间
    std::string toString() const;//转为年月日格式 只读方法
private:
    int64_t microSecondsSinceEpoch_;//存储时间戳的值
};