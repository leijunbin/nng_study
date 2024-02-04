#ifndef CORE_DEFS_H
#define CORE_DEFS_H

#include <stdint.h>

typedef struct nni_socket nni_sock;
typedef struct nni_dialer nni_dialer;

typedef uint64_t nni_time;    // Abs. time(ms)
typedef int32_t nni_duration; // Rel. time(ms)

#define NNI_ALLOC_STRUCT(s) nni_zalloc(sizeof(*s))

typedef struct nni_plat_mtx nni_mtx;
typedef struct nni_plat_cv nni_cv;
typedef struct nni_thr nni_thr;
typedef void (*nni_thr_func)(void *);

#endif // CORE_DEFS_H