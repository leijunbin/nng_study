#ifndef CORE_DEFS_H
#define CORE_DEFS_H

#include <stdint.h>

typedef struct nni_socket nni_sock;
typedef struct nni_dialer nni_dialer;

typedef uint64_t nni_time;    // Abs. time(ms)
typedef int32_t nni_duration; // Rel. time(ms)

// 自有对象命名
// typedef struct nni_plat_mtx nni_mtx

#endif // CORE_DEFS_H