#ifndef PLATFORM_POSIX_IMPL_H
#define PLATFORM_POSIX_IMPL_H

#include "platform/posix/posix_config.h"

extern int nni_plat_errno(int);

#include <pthread.h>

struct nni_plat_thr
{
    pthread_t tid;
    void (*func)(void *);
    void *arg;
};

struct nni_plat_mtx 
{
    pthread_mutex_t mtx;
};

struct nni_plat_cv
{
    pthread_cond_t cv;
    nni_plat_mtx *mtx;
};

// 初始化i/o后端
extern int nni_posix_pollq_sysinit(void);
extern void nni_posix_pollq_sysfini(void);
// 初始化
extern int nni_posix_resolv_sysinit(void);
extern void nni_posix_resolv_sysfini(void);

#endif // PLATFORM_POSIX_IMPL_H