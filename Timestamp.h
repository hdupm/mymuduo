#pragma once
#include <iostream>
//ʱ����
class Timestamp
{
public:
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpoch_);//�������Ĺ��캯������explicit ָ����������ʽת������
    static Timestamp now();//��ȡ��ǰʱ��
    std::string toString() const;//תΪ�����ո�ʽ ֻ������
private:
    int64_t microSecondsSinceEpoch_;//�洢ʱ�����ֵ
};