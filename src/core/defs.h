#ifndef CORE_DEFS_H
#define CORE_DEFS_H

#include <stdint.h>

#define NNI_ASSERT(x) \
    if(!(x)) \
    nni_panic("%s: %d: assert err: %s", __FILE__, __LINE__, #x)

typedef struct nni_socket nni_sock;
typedef struct nni_dialer nni_dialer;

typedef uint64_t nni_time;    // Abs. time(ms)
typedef int32_t nni_duration; // Rel. time(ms)

typedef void (*nni_cb)(void *);

#define NNI_TIME_NEVER ((nni_time) -1)
#define NNI_TIME_ZERO ((nni_time) 0)
#define NNI_SECOND (1000)

#define NNI_ALLOC_STRUCT(s) nni_zalloc(sizeof(*s))
#define NNI_FREE_STRUCT(s) nni_free((s), sizeof(*s))
#define NNI_ALLOC_STRUCTS(s, n) nni_zalloc(sizeof(*s) * n)
#define NNI_FREE_STRUCTS(s, n) nni_free((s), sizeof(*s) * n)

typedef struct nni_plat_mtx nni_mtx;
typedef struct nni_plat_cv nni_cv;
typedef struct nni_thr nni_thr;
typedef void (*nni_thr_func)(void *);

#endif // CORE_DEFS_H