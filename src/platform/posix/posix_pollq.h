#ifndef PLATFORM_POSIX_POLLQ_H
#define PLATFORM_POSIX_POLLQ_H

#include <poll.h>

typedef struct nni_posix_pfd nni_posix_pfd;
typedef void (*nni_posix_pfd_cb)(nni_posix_pfd *, unsigned, void *);

extern int nni_posix_pfd_init(nni_posix_pfd **, int);
extern void nni_posix_pfd_fini(nni_posix_pfd *);
extern int nni_posix_pfd_arm(nni_posix_pfd *, unsigned);
extern int nni_posix_pfd_fd(nni_posix_pfd *);
extern void nni_posix_pfd_close(nni_posix_pfd *);
extern void nni_posix_pfd_set_cb(nni_posix_pfd *, nni_posix_pfd_cb, void *);

#define NNI_POLL_IN ((unsigned) POLLIN)
#define NNI_POLL_OUT ((unsigned) POLLOUT)
#define NNI_POLL_HUP ((unsigned) POLLHUP)
#define NNI_POLL_ERR ((unsigned) POLLERR)
#define NNI_POLL_INVAL ((unsigned) POLLNVAL)

#endif // PLATFORM_POSIX_POLLQ_H