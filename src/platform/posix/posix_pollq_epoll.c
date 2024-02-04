#include "core/nng_impl.h"
#include "platform/posix/posix_pollq.h"

#include <sys/epoll.h>
#include <stdbool.h>
#include <fcntl.h>

typedef struct nni_posix_pollq nni_posix_pollq;

struct nni_posix_pollq
{
    nni_mtx mtx;
    int epfd; // epoll文件描述符
    int evfd; // epoll所监听事件的文件描述符
    bool close; // 工作线程是否退出
    nni_thr thr; // 工作线程
    nni_list reapq; // 链表
};

struct nni_posix_pfd
{
    nni_list_node node;
    nni_posix_pollq *pq;
    int fd;
    nni_posix_pfd_cb cb;
    void *arg;
    bool closed;
    bool closing;
    bool reap;
    unsigned events;
    nni_mtx mtx;
    nni_cv cv;
};

static nni_posix_pollq nni_posix_global_pollq;

int nni_posix_pfd_init(nni_posix_pfd **pfdp, int fd)
{
    nni_posix_pfd *pfd;
    nni_posix_pollq* pq;
    struct epoll_event ev;
    int rv;

    pq = &nni_posix_global_pollq;

    (void)fcntl(fd, F_SETFD, FD_CLOEXEC);
    (void)fcntl(fd, F_SETFL, O_NONBLOCK);

    if((pfd = NNI_ALLOC_STRUCT(pfd)) == NULL)
    {
        return NNG_ENOMEM;
    }
    

}