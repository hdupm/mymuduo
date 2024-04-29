#pragma once
#include <unistd.h>
#include <sys/syscall.h>//syscallͷ�ļ�

namespace CurrentThread
{
    extern __thread int t_cachedTid;//ȫ�ֱ�������һ�� extern�������ò���ͬһ���ļ��еı������ߺ���

    void cacheTid();

    inline int tid()//��������ֻ�ڵ�ǰ�ļ�������
    {
        if (__builtin_expect(t_cachedTid==0,0))//Ϊ0˵����û��ȡ����ǰ�̵߳�id __builtin_expect�������Ż�����
        {
            cacheTid();
        }
        return t_cachedTid;
    }
}
