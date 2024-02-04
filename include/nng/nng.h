// 对外暴露的结构和函数
#ifndef NNG_NNG_H
#define NNG_NNG_H

#include <stdint.h>

// nng共有类型
/*-- nng_socket --*/
typedef struct nng_socket_s
{
    uint32_t id;
    void *data;
} nng_socket;

/*-- nng_dialer --*/
typedef struct nng_dialer_s
{
    uint32_t id;
} nng_dialer;
/* 创建一个dialer对象(未开始) */
int nng_dialer_create(nng_dialer *, nng_socket, const char *);

// nng错误码
enum nng_errno_enum
{
    NNG_EINTR = 1,
    NNG_ENOMEM = 2,
    NNG_EINVAL = 3,
    NNG_EBUSY = 4,
    NNG_ETIMEDOUT = 5,
    NNG_ECONNREFUSED = 6,
    NNG_CLOSED = 7,
    NNG_EAGAIN = 8,
    NNG_ENOTSUP = 9,
    NNG_EADDRINUSE = 10,
    NNG_ESTATE = 11,
    NNG_ENOENT = 12,
    NNG_EPROTO = 13,
    NNG_EUNREACHABLE = 14,
    NNG_EADDRINVAL = 15,
    NNG_EPERM = 16,
    NNG_EMSGSIZE = 17,
    NNG_ECONNABORTED = 18,
    NNG_ECONNRESET = 19,
    NNG_ECANCELED = 20,
    NNG_ENOFILES = 21,
    NNG_ENOSPC = 22,
    NNG_EEXIST = 23,
    NNG_EREADONLY = 24,
    NNG_EWRITEONLY = 25,
    NNG_ECRYPTO = 26,
    NNG_EPEERAUTH = 27,
    NNG_ENOARG = 28,
    NNG_EAMBIGOUS = 29,
    NNG_EBADTYPE = 30,
    NNG_ECONNSHUT = 31,
    NNG_EINTERNAL = 1000,
    NNG_ESYSERR = 0x10000000,
    NNG_ETRANERR = 0x20000000
};

#endif // NNG_NNG_H