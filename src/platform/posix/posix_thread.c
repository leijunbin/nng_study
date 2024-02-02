#include "core/nng_impl.h"

// #ifdef NNG_PLATFORM_POSIX

#include <pthread.h>

// PTHREAD_MUTEX_INITIALIZER为pthread初始化的却省值
static pthread_mutex_t nni_plat_init_lock = PTHREAD_MUTEX_INITIALIZER;
static volatile int nni_plat_inited = 0;
static int nni_plat_forked = 0;

// 条件变量信息
pthread_condattr_t nni_cvaddr;
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
        
    }
}

int nni_plat_init(int (*helper)(void))
{

}

// #endif