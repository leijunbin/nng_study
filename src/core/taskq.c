#include "core/nng_impl.h"

typedef struct nni_taskq_thr
{
    nni_taskq *tqt_tq;
    nni_thr tqt_thread;
} nni_taskq_thr;

struct nni_taskq
{
    nni_list tq_tasks;
    nni_mtx tq_mtx;
    nni_cv tq_sched_cv;
    nni_cv tq_wait_cv;
    nni_taskq_thr *tq_threads;
    int tq_nthreads;
    bool tq_run;
};

static nni_taskq *nni_taskq_systq = NULL;

static void nni_taskq_thread(void *self)
{
    nni_taskq_thr *thr = self;
    nni_taskq *tq = thr->tqt_tq;
    nni_task *task;

    nni_thr_set_name(NULL, "nng:task");

    nni_mtx_lock(&tq->tq_mtx);
    for(;;)
    {
        if((task = nni_list_first(&tq->tq_tasks)) != NULL)
        {
            nni_list_remove(&tq->tq_tasks, task);
            nni_mtx_unlock(&tq->tq_mtx);

            task->task_cb(task->task_arg);

            nni_mtx_lock(&task->task_mtx);
            task->task_busy--;
            if(task->task_busy == 0)
            {
                nni_cv_wake(&task->task_cv);
            }
            nni_mtx_unlock(&task->task_mtx);

            nni_mtx_lock(&tq->tq_mtx);

            continue;
        }

        if(!tq->tq_run)
        {
            break;
        }
        nni_cv_wait(&tq->tq_sched_cv);
    }
    nni_mtx_unlock(&tq->tq_mtx);
}

int nni_taskq_init(nni_taskq **tqp, int nthr)
{
    nni_taskq *tq;

    if((tq = NNI_ALLOC_STRUCT(tq)) == NULL)
    {
        return NNG_ENOMEM;
    }

    if((tq->tq_threads = NNI_ALLOC_STRUCTS(tq->tq_threads, nthr)) == NULL)
    {
        NNI_FREE_STRUCT(tq);
        return NNG_ENOMEM;
    }
    tq->tq_nthreads = nthr;
    NNI_LIST_INIT(&tq->tq_tasks, nni_task, task_node);

    nni_mtx_init(&tq->tq_mtx);
    nni_cv_init(&tq->tq_sched_cv, &tq->tq_mtx);
    nni_cv_init(&tq->tq_wait_cv, &tq->tq_mtx);

    for(int i = 0; i < nthr; i++)
    {
        int rv;
        tq->tq_threads[i].tqt_tq = tq;
        rv = nni_thr_init(&tq->tq_threads[i].tqt_thread, nni_taskq_thread, &tq->tq_threads[i]);
        if(rv != 0)
        {
            nni_taskq_fini(tq);
            return rv;
        }
    }
    tq->tq_run = true;
    for(int i = 0; i < tq->tq_nthreads; i++)
    {
        nni_thr_run(&tq->tq_threads[i].tqt_thread);
    }
    *tqp = tq;
    return 0;
}

void nni_taskq_fini(nni_taskq *tq)
{
    if(tq == NULL)
    {
        return;
    }

    if(tq->tq_run)
    {
        nni_mtx_lock(&tq->tq_mtx);
        tq->tq_run = false;
        nni_cv_wake(&tq->tq_sched_cv);
        nni_mtx_unlock(&tq->tq_mtx);
    }

    for(int i = 0; i < tq->tq_nthreads; i++)
    {
        nni_thr_fini(&tq->tq_threads[i].tqt_thread);
    }
    nni_cv_fini(&tq->tq_wait_cv);
    nni_cv_fini(&tq->tq_sched_cv);
    nni_mtx_fini(&tq->tq_mtx);
    NNI_FREE_STRUCTS(tq->tq_threads, tq->tq_nthreads);
    NNI_FREE_STRUCT(tq);
}