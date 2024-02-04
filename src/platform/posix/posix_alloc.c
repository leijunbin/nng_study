#include "core/nng_impl.h"

#include <stdlib.h>

void *nni_alloc(size_t sz)
{
    return (sz > 0 ? malloc(sz) : NULL);
}

void *nni_zalloc(size_t sz)
{
    return (sz > 0 ? calloc(1, sz) : NULL);
}

void nni_free(void *ptr, size_t size)
{
    free(ptr);
}