#include "core/nng_impl.h"

// #ifdef NNG_PLATFORM_POSIX

#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>

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

static void nni_pthread_mutex_lock(pthread_mutex_t *m)
{
    int rv;
    if((rv = pthread_mutex_lock(m)) != 0)
    {
        nni_panic("pthread_mutex_lock: %s", strerror(rv));
    }
}

static void nni_pthread_mutex_unlock(pthread_mutex_t *m)
{
    int rv;
    if((rv = pthread_mutex_unlock(m)) != 0)
    {
        nni_panic("pthread_mutex_unloack: %s", strerror(rv));
    }
}

static void nni_pthread_cond_broadcast(pthread_cond_t *c)
{
    int rv;
    if((rv = pthread_cond_broadcast(c)) != 0)
    {
        nni_panic("pthread_cond_broadcast: %s", strerror(rv));
    }
}

static void nni_pthread_cond_signal(pthread_cond_t *c)
{
    int rv;
    if((rv = pthread_cond_signal(c)) != 0)
    {
        nni_panic("pthread_cond_signal: %s", strerror(rv));
    }
}

static void nni_pthread_cond_wait(pthread_cond_t *c, pthread_mutex_t *m)
{
    int rv;
    if((rv = pthread_cond_wait(c, m)) != 0)
    {
        nni_panic("pthread_cond_wait: %s", strerror(rv));
    }
}

static int nni_pthread_cond_timedwait(pthread_cond_t *c, pthread_mutex_t *m, struct timespec *ts)
{
    int rv;

    switch((rv = pthread_cond_timedwait(c, m, ts)))
    {
        case 0:
            return 0;
        case ETIMEDOUT:
        case EAGAIN:
            return NNG_ETIMEDOUT;
    }
    nni_panic("pthread_cond_timedwait: %s", strerror(rv));
    return NNG_EINVAL;
}

void nni_plat_mtx_lock(nni_plat_mtx *mtx)
{
    nni_pthread_mutex_lock(&mtx->mtx);
}

void nni_plat_mtx_unlock(nni_plat_mtx *mtx)
{
    nni_pthread_mutex_unlock(&mtx->mtx);
}

void nni_plat_cv_init(nni_plat_cv *cv, nni_plat_mtx *mtx)
{
    while(pthread_cond_init(&cv->cv, &nni_cvattr) != 0)
    {
        nni_msleep(10);
    }
    cv->mtx = mtx;
}

void nni_plat_cv_wake(nni_plat_cv *cv)
{
    nni_pthread_cond_broadcast(&cv->cv);
}

void nni_plat_cv_wake1(nni_plat_cv *cv)
{
    nni_pthread_cond_signal(&cv->cv);
}

void nni_plat_cv_wait(nni_plat_cv *cv)
{
    nni_pthread_cond_wait(&cv->cv, &cv->mtx->mtx);
}

int nni_plat_cv_until(nni_plat_cv *cv, nni_time until)
{
    struct timespec ts;

    ts.tv_sec = until / 1000;
    ts.tv_nsec = (until % 1000) * 1000000;

    return nni_pthread_cond_timedwait(&cv->cv, &cv->mtx->mtx, &ts);
}

void nni_plat_cv_fini(nni_plat_cv *cv)
{
    int rv;

    if((rv = pthread_cond_destroy(&cv->cv)) != 0)
    {
        nni_panic("pthread_cond_destroy: %s", strerror(rv));
    }
    cv->mtx = NULL;
}

static void *nni_plat_thr_main(void *arg)
{
    nni_plat_thr *thr = arg;
    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    (void) pthread_sigmask(SIG_BLOCK, &set, NULL);

    thr->func(thr->arg);

    return NULL;
}

int nni_plat_thr_init(nni_plat_thr *thr, void (*fn)(void *), void *arg)
{
    int rv;

    thr->func = fn;
    thr->arg = arg;
    rv = pthread_create(&thr->tid, &nni_thrattr, nni_plat_thr_main, thr);

    if(rv != 0)
    {
        return NNG_ENOMEM;
    }

    return 0;
}

void nni_plat_thr_fini(nni_plat_thr *thr)
{
    int rv;
    
    if((rv = pthread_join(thr->tid, NULL)))
    {
        nni_panic("pthread_join: %s", strerror(rv));
    }
}

bool nni_plat_thr_is_self(nni_plat_thr *thr)
{
    return pthread_self() == thr->tid;
}

void nni_plat_thr_set_name(nni_plat_thr *thr, const char *name)
{
    if(thr == NULL)
    {
        pthread_setname_np(pthread_self(), name);
    }
    else
    {
        pthread_setname_np(thr->tid, name);
    }
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

    // 创建i/o复用后端
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