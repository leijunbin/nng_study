#include "core/nng_impl.h"

#include <stdio.h>
#include <stdlib.h>

void nni_plat_abort(void)
{
    abort();
}

void nni_plat_println(const char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
}