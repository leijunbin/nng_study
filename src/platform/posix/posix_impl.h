#ifndef PLATFORM_POSIX_IMPL_H
#define PLATFORM_POSIX_IMPL_H

#include "platform/posix/posix_config.h"

#include <pthread.h>

struct nni_plat_mtx 
{
    pthread_mutex_t mtx;
};

#endif // PLATFORM_POSIX_IMPL_H