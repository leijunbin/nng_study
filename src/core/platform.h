#ifndef CORE_PLATFORM_H
#define CORE_PLATFORM_H

#include <stddef.h>
#include <stdbool.h>

// 不同平台的调试支持
extern void nni_plat_abort();
extern void nni_plat_println(const char *);
extern void nni_plat_printf(const char *, ...);
extern const char *nni_plat_strerror(int);

// 不同平台的内存管理支持
extern void *nni_alloc(size_t);
extern void *nni_zalloc(size_t);
extern void nni_free(void *, size_t);

// 不同平台的时钟抽象
extern nni_time nni_clock(void);
extern void nni_msleep(nni_duration);

// 
extern int nni_plat_init(int (*)(void));

typedef struct nni_plat_mtx nni_plat_mtx;
typedef struct nni_plat_cv nni_plat_cv;
typedef struct nni_plat_thr nni_plat_thr;

// 不同平台线程同步抽象
extern void nni_plat_mtx_init(nni_plat_mtx *);
extern void nni_plat_mtx_fini(nni_plat_mtx *);

extern void nni_plat_mtx_lock(nni_plat_mtx *);
extern void nni_plat_mtx_unlock(nni_plat_mtx *);

extern void nni_plat_cv_init(nni_plat_cv *, nni_plat_mtx *);
extern void nni_plat_cv_fini(nni_plat_cv *);
extern void nni_plat_cv_wake(nni_plat_cv *);
extern void nni_plat_cv_wake1(nni_plat_cv *);
extern void nni_plat_cv_wait(nni_plat_cv *);
extern int nni_plat_cv_until(nni_plat_cv *, nni_time);

extern int nni_plat_thr_init(nni_plat_thr *, void (*)(void *), void *);
extern void nni_plat_thr_fini(nni_plat_thr *);
extern bool nni_plat_thr_is_self(nni_plat_thr *);
extern void nni_plat_thr_set_name(nni_plat_thr *, const char *);

// 平台选择代码
#if 1
#include "platform/posix/posix_impl.h"
#endif

#endif