#include "core/nng_impl.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void nni_plat_abort(void)
{
    abort();
}

void nni_plat_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    (void) vprintf(fmt, ap);
    va_end(ap);
}

void nni_plat_println(const char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
}

const char *nni_plat_strerror(int errnum)
{
    if(errnum > NNG_ESYSERR)
    {
        errnum -= NNG_ESYSERR;
    }

    return strerror(errnum);
}

static struct 
{
    int posix_err;
    int nng_err;
} nni_plat_errnos[] = {
    {EINTR, NNG_EINTR},
    {EINVAL, NNG_EINVAL},
	{ENOMEM, NNG_ENOMEM},
	{EACCES, NNG_EPERM},
	{EADDRINUSE, NNG_EADDRINUSE},
	{EADDRNOTAVAIL, NNG_EADDRINVAL},
	{EAFNOSUPPORT,	NNG_ENOTSUP},
	{EAGAIN, NNG_EAGAIN},
	{EBADF, NNG_ECLOSED},
	{EBUSY,	NNG_EBUSY},
	{ECONNABORTED, NNG_ECONNABORTED},
	{ECONNREFUSED, NNG_ECONNREFUSED},
	{ECONNRESET, NNG_ECONNRESET},
	{EHOSTUNREACH, NNG_EUNREACHABLE},
	{ENETUNREACH, NNG_EUNREACHABLE},
	{ENAMETOOLONG, NNG_EINVAL},
	{ENOENT, NNG_ENOENT},
	{ENOBUFS, NNG_ENOMEM},
	{ENOPROTOOPT, NNG_ENOTSUP},
	{ENOSYS, NNG_ENOTSUP},
	{ENOTSUP, NNG_ENOTSUP},
	{EPERM, NNG_EPERM},
	{EPIPE, NNG_ECLOSED},
	{EPROTO, NNG_EPROTO},
	{EPROTONOSUPPORT, NNG_ENOTSUP},
#ifdef  ETIME   // Found in STREAMs, not present on all systems.
	{ETIME, NNG_ETIMEDOUT},
#endif
	{ETIMEDOUT, NNG_ETIMEDOUT},
	{EWOULDBLOCK, NNG_EAGAIN},
	{ENOSPC, NNG_ENOSPC},
	{EFBIG, NNG_ENOSPC},
	{EDQUOT, NNG_ENOSPC},
	{ENFILE, NNG_ENOFILES},
	{EMFILE, NNG_ENOFILES},
	{EEXIST, NNG_EEXIST},
    {0, 0}
};

int nni_plat_errno(int errnum)
{
    int i;

    if(errnum == 0)
    {
        return 0;
    }

    if (errnum == EFAULT)
    {
        nni_panic("System EFAULT encountered");
    }

    for(i = 0; nni_plat_errnos[i].nng_err != 0; i++)
    {
        if(errnum == nni_plat_errnos[i].posix_err)
        {
            return nni_plat_errnos[i].nng_err;
        }
    }

    return NNG_ESYSERR + errnum;
}