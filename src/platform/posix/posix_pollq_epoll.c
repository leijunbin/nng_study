#include "core/nng_impl.h"
#include "platform/posix/posix_pollq.h"

#include <errno.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/eventfd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

typedef struct nni_posix_pollq nni_posix_pollq;

#define NNI_MAX_EPOLL_EVENTS 64

#define NNI_EPOLL_FLAGS ((unsigned) EPOLLONESHOT | (unsigned) EPOLLERR)

struct nni_posix_pollq
{
    nni_mtx mtx;
    int epfd; // epoll文件描述符
    int evfd; // 唤醒epoll所在线程的文件描述符
    bool close; // epoll线程是否关闭
    nni_thr thr; // 工作线程
    nni_list reapq; // 删除元素列表
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

    nni_mtx_init(&pfd->mtx);
    nni_cv_init(&pfd->cv, &pq->mtx);

    pfd->pq = pq;
    pfd->fd = fd;
    pfd->cb = NULL;
    pfd->arg = NULL;
    pfd->events = 0;
    pfd->closing = false;
    pfd->closed = false;

    NNI_LIST_NODE_INIT(&pfd->node);

    memset(&ev, 0, sizeof(ev));
    ev.events = 0;
    ev.data.ptr = pfd;

    if(epoll_ctl(pq->epfd, EPOLL_CTL_ADD, fd, &ev) != 0)
    {
        rv = nni_plat_errno(errno);
        nni_cv_fini(&pfd->cv);
        nni_mtx_fini(&pfd->mtx);
        NNI_FREE_STRUCT(pfd);
        return rv;
    }

    *pfdp = pfd;

    return 0;
}

int nni_posix_pfd_arm(nni_posix_pfd *pfd, unsigned events)
{
    nni_posix_pollq *pq = pfd->pq;

    nni_mtx_lock(&pfd->mtx);

    if(!pfd->closing)
    {
        struct epoll_event ev;
        pfd->events |= events;
        events = pfd->events;

        memset(&ev, 0, sizeof(ev));
        ev.events = events | NNI_EPOLL_FLAGS;
        ev.data.ptr = pfd;

        if(epoll_ctl(pq->epfd, EPOLL_CTL_MOD, pfd->fd, &ev) != 0)
        {
            int rv = nni_plat_errno(errno);
            nni_mtx_unlock(&pfd->mtx);
            return rv;
        }
    }

    nni_mtx_unlock(&pfd->mtx);
    return 0;
}

int nni_posix_pfd_fd(nni_posix_pfd *pfd)
{
    return pfd->fd;
}

void nni_posix_pfd_set_cb(nni_posix_pfd *pfd, nni_posix_pfd_cb cb, void *arg)
{
    nni_mtx_lock(&pfd->mtx);
    
    pfd->cb = cb;
    pfd->arg = arg;

    nni_mtx_unlock(&pfd->mtx);
}

void nni_posix_pfd_close(nni_posix_pfd *pfd)
{
    nni_mtx_lock(&pfd->mtx);
    
    if(!pfd->closing)
    {
        nni_posix_pollq *pq = pfd->pq;
        struct epoll_event ev;
        pfd->closing = true;

        (void) shutdown(pfd->fd, SHUT_RDWR);
        (void) epoll_ctl(pq->epfd, EPOLL_CTL_DEL, pfd->fd, &ev);
    }

    nni_mtx_unlock(&pfd->mtx);
}

void nni_posix_pfd_fini(nni_posix_pfd *pfd)
{
    nni_posix_pollq *pq = pfd->pq;

    nni_posix_pfd_close(pfd);

    NNI_ASSERT(!nni_thr_is_self(&pq->thr));

    uint64_t one = 1;

    nni_mtx_lock(&pq->mtx);
    nni_list_append(&pq->reapq, pfd);

    // 唤醒epoll所在线程
    if(write(pq->evfd, &one, sizeof(one)) != sizeof(one))
    {
        nni_panic("BUG! write to epoll fd incorrect!");
    }

    while(!pfd->closed)
    {
        nni_cv_wait(&pfd->cv);
    }
    nni_mtx_unlock(&pq->mtx);

    (void)close(pfd->fd);
    nni_cv_fini(&pfd->cv);
    nni_mtx_fini(&pfd->mtx);
    NNI_FREE_STRUCT(pfd);
}

static void nni_posix_pollq_reap(nni_posix_pollq *pq)
{
    nni_posix_pfd *pfd;
    nni_mtx_lock(&pq->mtx);
    while((pfd = nni_list_first(&pq->reapq)) != NULL)
    {
        nni_list_remove(&pq->reapq, pfd);
        pfd->closed = true;
        nni_cv_wake(&pfd->cv);
    }
    nni_mtx_unlock(&pq->mtx);
}

static void nni_posix_poll_thr(void *arg)
{
    nni_posix_pollq *pq = arg;
    struct epoll_event events[NNI_MAX_EPOLL_EVENTS];

    for(;;)
    {
        int n;
        bool reap = false;

        n = epoll_wait(pq->epfd, events, NNI_MAX_EPOLL_EVENTS, -1);
        if(n < 0 && errno == EBADF)
        {
            return;
        }

        for(int i = 0; i < n; ++i)
        {
            const struct epoll_event *ev;

            ev = &events[i];
            if(ev->data.ptr == NULL && (ev->events & (unsigned)POLLIN)) 
            {
                uint64_t clear;
                if(read(pq->evfd, &clear, sizeof(clear)) != sizeof(clear))
                {
                    nni_panic("read from evfd incorrect!");
                }
            }
            else
            {
                nni_posix_pfd *pfd = ev->data.ptr;
                nni_posix_pfd_cb cb;
                void *cbarg;
                unsigned mask;

                mask = ev->events & ((unsigned) EPOLLIN | (unsigned) EPOLLOUT | (unsigned) EPOLLERR);

                nni_mtx_lock(&pfd->mtx);
                pfd->events &= ~mask;
                cb = pfd->cb;
                cbarg = pfd->arg;
                nni_mtx_unlock(&pfd->mtx);

                if(cb != NULL)
                {
                    cb(pfd, mask, cbarg);
                }
            }
        }

        if(reap)
        {
            nni_posix_pollq_reap(pq);
            nni_mtx_lock(&pq->mtx);
            if(pq->close)
            {
                nni_mtx_unlock(&pq->mtx);
                return;
            }
            nni_mtx_unlock(&pq->mtx);
        }
    }
}

static void nni_posix_pollq_destroy(nni_posix_pollq *pq)
{
    uint64_t one = 1;

    nni_mtx_lock(&pq->mtx);
    pq->close = true;
    if(write(pq->evfd, &one, sizeof(one) != sizeof(one)))
    {
        nni_panic("BUG! unable to write to evfd!");
    }
    nni_mtx_unlock(&pq->mtx);

    nni_thr_fini(&pq->thr);

    close(pq->evfd);
    close(pq->epfd);

    nni_mtx_fini(&pq->mtx);
}

static int nni_posix_pollq_add_eventfd(nni_posix_pollq *pq)
{
    struct epoll_event ev;
    int fd;

    memset(&ev, 0, sizeof(ev));

    if(fd = eventfd(0, EFD_NONBLOCK) < 0)
    {
        return nni_plat_errno(errno);
    }
    (void)fcntl(fd, F_SETFD, FD_CLOEXEC);
    (void)fcntl(fd, F_SETFL, O_NONBLOCK);

    ev.events = EPOLLIN;
    ev.data.ptr = 0;

    if(epoll_ctl(pq->epfd, EPOLL_CTL_ADD, fd, &ev) != 0)
    {
        (void)close(fd);
        return nni_plat_errno(errno);
    }
    pq->evfd = fd;
    return 0;
}

static int nni_posix_pollq_create(nni_posix_pollq *pq)
{
    int rv;

    if((pq->epfd = epoll_create1(EPOLL_CLOEXEC)) < 0)
    {
        return nni_plat_errno(errno);
    }

    pq->close = false;

    NNI_LIST_INIT(&pq->reapq, nni_posix_pfd, node);
    nni_mtx_init(&pq->mtx);

    if((rv = nni_posix_pollq_add_eventfd(pq)) != 0)
    {
        (void)close(pq->epfd);
        nni_mtx_fini(&pq->mtx);
        return rv;
    }
    if((rv = nni_thr_init(&pq->thr, nni_posix_poll_thr, pq)) != 0)
    {
        (void)close(pq->epfd);
        (void)close(pq->evfd);
        nni_mtx_fini(&pq->mtx);
        return rv;
    }
    nni_thr_set_name(&pq->thr, "nng:poll:epoll");
    nni_thr_run(&pq->thr);
    return 0;
}

int nni_posix_pollq_sysinit(void)
{
    return nni_posix_pollq_create(&nni_posix_global_pollq);
}

void nni_posix_pollq_sysfini(void)
{
    nni_posix_pollq_destroy(&nni_posix_global_pollq);
}