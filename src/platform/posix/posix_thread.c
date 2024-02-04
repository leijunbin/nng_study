#include "core/nng_impl.h"

// #ifdef NNG_PLATFORM_POSIX

#include <pthread.h>

// PTHREAD_MUTEX_INITIALIZER为pthread初始化的却省值
// 此互斥锁为初始化过程中的互斥锁
static pthread_mutex_t nni_plat_init_lock = PTHREAD_MUTEX_INITIALIZER;
static volatile int nni_plat_inited = 0;
static int nni_plat_forked = 0;

// 条件变量信息
pthread_condattr_t nni_cvattr;
// 互斥锁信息
pthread_mutexattr_t nni_mxattr;
// 线程信息
pthread_attr_t nni_thrattr;

void nni_plat_mtx_init(nni_plat_mtx *mtx)
{
    // 就是在大部分支持posix接口标准的内核(如linux)上，pthread_mutex_init
    // 接口在传入空互斥锁信息时会创建一个默认信息的互斥锁同时不分配内存，在这种
    // 情况下几乎不会失败；但是在bsd内核（著名的就是mac os上）传入空互斥锁
    // 信息时也会创建默认信息的互斥锁但是会分配内存，这导致如果可分配内存不
    // 足，在bsd内核下互斥锁创建就不会成功，影响程序运行
    while((pthread_mutex_init(&mtx->mtx, &nni_mxattr) != 0) &&
            (pthread_mutex_init(&mtx->mtx, NULL) != 0))
    {
        nni_msleep(10);
    }
}

void nni_plat_mtx_fini(nni_plat_mtx *mtx)
{
    (void)pthread_mutex_destroy(&mtx->mtx);
}

int nni_plat_init(int (*helper)(void))
{
    int rv;

    if(nni_plat_forked)
    {
        nni_panic("nng is not fork-reentrant safe");
    }

    if(nni_plat_inited)
    {
        return 0;
    }

    pthread_mutex_lock(&nni_plat_init_lock);
    if(nni_plat_inited)
    {
        pthread_mutex_unlock(&nni_plat_init_lock);
        return 0;
    }

    if((pthread_mutexattr_init(&nni_mxattr) != 0) ||
        (pthread_condattr_init(&nni_cvattr) != 0) ||
        (pthread_attr_init(&nni_thrattr) != 0))
    {
        pthread_mutex_unlock(&nni_plat_init_lock);
        return NNG_ENOMEM;
    }

    // 当在同一线程中重复锁定时，会返回错误，不会阻塞
    (void)pthread_mutexattr_settype(&nni_mxattr, PTHREAD_MUTEX_ERRORCHECK);

    if((rv = nni_posix_pollq_sysinit()) != 0)
    {
        pthread_mutex_unlock(&nni_plat_init_lock);
        pthread_mutexattr_destroy(&nni_mxattr);
        pthread_condattr_destroy(&nni_cvattr);
        pthread_attr_destroy(&nni_thrattr);
        return rv;
    }
}


// #endif