#pragma once
#include <unistd.h>
#include <sys/syscall.h>//syscall头文件

namespace CurrentThread
{
    extern __thread int t_cachedTid;//全局变量声明一下 extern可以引用不在同一个文件中的变量或者函数

    void cacheTid();

    inline int tid()//内联函数只在当前文件起作用
    {
        if (__builtin_expect(t_cachedTid==0,0))//为0说明还没获取过当前线程的id __builtin_expect是性能优化函数
        {
            cacheTid();
        }
        return t_cachedTid;
    }
}
