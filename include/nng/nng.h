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

#endif // NNG_NNG_H