#include "core/nng_impl.h"

#include <stdarg.h>

void nni_panic(const char *fmt, ...)
{
    char buf[100];
    char fbuf[93];
    va_list va;

    va_start(va, fmt);
    (void)vsnprintf(fbuf, sizeof(fbuf), fmt, va);
    va_end(va);

    (void)snprintf(buf, sizeof(buf), "panic: %s", fbuf);

    nni_println(buf);

    nni_plat_abort();
}

void nni_println(const char *msg)
{
    nni_plat_println(msg);
}