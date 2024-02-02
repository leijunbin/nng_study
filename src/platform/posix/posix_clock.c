#include "core/nng_impl.h"

#include <time.h>

#if !defined(NNG_USE_GETTIMEOFDAY)

#include <string.h>

nni_time nni_clock(void)
{
    struct timespec ts;
    nni_time msec;

    if(clock_gettime(NNG_USE_CLOCKID, &ts))
    {
        nni_panic("clock_gettime failed: %s", strerror(errno));
    }

    msec = ts.tv_sec;
    msec *= 1000;
    msec += (ts.tv_nsec / 1000000);

    return msec;
}

void nni_msleep(nni_duration ms)
{
    struct timespec ts;

    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;

    // 以便睡眠时被中断能够再次进入睡眠
    while(ts.tv_sec || ts.tv_nsec)
    {
        // 指定睡眠时间和剩余睡眠时间
        if(nanosleep(&ts, &ts) == 0)
        {
            break;
        }
    }
}

// 跨平台、跨架构睡眠函数可能会出现两个问题：
// 1. posix接口不强制支持nanosleep，导致睡眠的精度不够（只有秒级）。
// 2. 可以使用pthread库利用条件变量去实现，但是这样会另行分配内存，且
//    会强制使用pthread库，这样在一些不想使用到pthread库的场景可能会
//    强制使用。
// 3. pthread时间设置还有一个缺陷。对于多核系统，不同核心的频率不同，
//    时钟周期不同，时间也不同，故大部分多核的时间寄存器都采用存储偏移
//    值的方式实现，内核更改时间需要计算这个偏移值。这样的底层实现导致
//    如果通过posix接口修改时间，会有小概率导致不同核心相对时间不准确，
//    使得计时不够精准。（此条带有猜测）
// 4. 相关时钟的现代实现的可能缺失也是一个问题，导致计时与执行并不在同
//    一个核心之上，在linux 2.2之后才改进了相关问题（现代时钟）。
// 综合考虑使用poll i/o复用的方式实现。
// void nni_msleep(nni_duration ms)
// {

// }

#else // NNG_USE_GETTIMEOFDAY

#endif