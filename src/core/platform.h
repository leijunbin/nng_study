#ifndef CORE_PLATFORM_H
#define CORE_PLATFORM_H

// 不同平台的调试支持
extern void nni_plat_abort();
extern void nni_plat_println(const char *);

// 不同平台的时钟抽象
extern nni_time nni_clock(void);
extern void nni_msleep(nni_duration);

// 
extern int nni_plat_init(int (*)(void));

typedef struct nni_plat_mtx nni_plat_mtx;

// 平台选择代码
#if 1
#include "platform/posix/posix_impl.h"
#endif

#endif