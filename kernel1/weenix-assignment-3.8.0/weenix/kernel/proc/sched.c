/******************************************************************************/
/* Important Spring 2021 CSCI 402 usage information:                          */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/*         53616c7465645f5fd1e93dbf35cbffa3aef28f8c01d8cf2ffc51ef62b26a       */
/*         f9bda5a68e5ed8c972b17bab0f42e24b19daa7bd408305b1f7bd6c7208c1       */
/*         0e36230e913039b3046dd5fd0ba706a624d33dbaa4d6aab02c82fe09f561       */
/*         01b0fd977b0051f0b0ce0c69f7db857b1b5e007be2db6d42894bf93de848       */
/*         806d9152bd5715e9                                                   */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

#include "globals.h"
#include "errno.h"

#include "main/interrupt.h"

#include "proc/sched.h"
#include "proc/kthread.h"

#include "util/init.h"
#include "util/debug.h"

static ktqueue_t kt_runq;

static __attribute__((unused)) void
sched_init(void)
{
        sched_queue_init(&kt_runq);
}
init_func(sched_init);



/*** PRIVATE KTQUEUE MANIPULATION FUNCTIONS ***/
/**
 * Enqueues a thread onto a queue.
 *
 * @param q the queue to enqueue the thread onto
 * @param thr the thread to enqueue onto the queue
 */
void
ktqueue_enqueue(ktqueue_t *q, kthread_t *thr)
{
        KASSERT(!thr->kt_wchan);
        list_insert_head(&q->tq_list, &thr->kt_qlink);
        thr->kt_wchan = q;
        q->tq_size++;
}

/**
 * Dequeues a thread from the queue.
 *
 * @param q the queue to dequeue a thread from
 * @return the thread dequeued from the queue
 */
kthread_t *
ktqueue_dequeue(ktqueue_t *q)
{
        kthread_t *thr;
        list_link_t *link;

        if (list_empty(&q->tq_list))
                return NULL;

        link = q->tq_list.l_prev;
        thr = list_item(link, kthread_t, kt_qlink);
        list_remove(link);
        thr->kt_wchan = NULL;

        q->tq_size--;

        return thr;
}

/**
 * Removes a given thread from a queue.
 *
 * @param q the queue to remove the thread from
 * @param thr the thread to remove from the queue
 */
static void
ktqueue_remove(ktqueue_t *q, kthread_t *thr)
{
        KASSERT(thr->kt_qlink.l_next && thr->kt_qlink.l_prev);
        list_remove(&thr->kt_qlink);
        thr->kt_wchan = NULL;
        q->tq_size--;
}

/*** PUBLIC KTQUEUE MANIPULATION FUNCTIONS ***/
void
sched_queue_init(ktqueue_t *q)
{
        list_init(&q->tq_list);
        q->tq_size = 0;
}

int
sched_queue_empty(ktqueue_t *q)
{
        return list_empty(&q->tq_list);
}

/*
 * Similar to sleep on, but the sleep can be cancelled.
 *
 * Don't forget to check the kt_cancelled flag at the correct times.
 *
 * Use the private queue manipulation functions above.
 */
int
sched_cancellable_sleep_on(ktqueue_t *q)
{
        // NOT_YET_IMPLEMENTED("PROCS: sched_cancellable_sleep_on");

        /* if the thread was cancelled, return -EINTR */
        if (curthr->kt_cancelled == 1){
                dbg(DBG_PRINT, "(GRADING1C)\n");
                return -EINTR;
        }

        /* make sleep cancellable */
        curthr->kt_state = KT_SLEEP_CANCELLABLE;
        /* enqueue the current thread */
        ktqueue_enqueue(q, curthr);
        /* schedule context switch */
        sched_switch();

        /* if the thread was cancelled, return -EINTR */
        if (curthr->kt_cancelled == 1){
                dbg(DBG_PRINT, "(GRADING1C)\n");
                return -EINTR;
        }

        dbg(DBG_PRINT, "(GRADING1A)\n");

        return 0;
}

/*
 * If the thread's sleep is cancellable, we set the kt_cancelled
 * flag and remove it from the queue. Otherwise, we just set the
 * kt_cancelled flag and leave the thread on the queue.
 *
 * Remember, unless the thread is in the KT_NO_STATE or KT_EXITED
 * state, it should be on some queue. Otherwise, it will never be run
 * again.
 */
void
sched_cancel(struct kthread *kthr)
{
        // NOT_YET_IMPLEMENTED("PROCS: sched_cancel");
        
        /* if this thread is of Illegal or Exited state, just return */
        if (kthr->kt_state == KT_NO_STATE || kthr->kt_state == KT_EXITED){
                dbg(DBG_PRINT, "(GRADING1C)\n");
                return;
        }

        /* set kt_cancelled flag */
        kthr->kt_cancelled = 1;

        /* if the thread's sleep is cancellable */
        if (kthr->kt_state == KT_SLEEP_CANCELLABLE){
                /* enqueue it onto run queue */
                sched_make_runnable(kthr);
                dbg(DBG_PRINT, "(GRADING1C)\n");
        }
        /* otherwise, just leave it on the queue */

        dbg(DBG_PRINT, "(GRADING1C)\n");
}

/*
 * In this function, you will be modifying the run queue, which can
 * also be modified from an interrupt context. In order for thread
 * contexts and interrupt contexts to play nicely, you need to mask
 * all interrupts before reading or modifying the run queue and
 * re-enable interrupts when you are done. This is analagous to
 * locking a mutex before modifying a data structure shared between
 * threads. Masking interrupts is accomplished by setting the IPL to
 * high.
 *
 * Once you have masked interrupts, you need to remove a thread from
 * the run queue and switch into its context from the currently
 * executing context.
 *
 * If there are no threads on the run queue (assuming you do not have
 * any bugs), then all kernel threads are waiting for an interrupt
 * (for example, when reading from a block device, a kernel thread
 * will wait while the block device seeks). You will need to re-enable
 * interrupts and wait for one to occur in the hopes that a thread
 * gets put on the run queue from the interrupt context.
 *
 * The proper way to do this is with the intr_wait call. See
 * interrupt.h for more details on intr_wait.
 *
 * Note: When waiting for an interrupt, don't forget to modify the
 * IPL. If the IPL of the currently executing thread masks the
 * interrupt you are waiting for, the interrupt will never happen, and
 * your run queue will remain empty. This is very subtle, but
 * _EXTREMELY_ important.
 *
 * Note: Don't forget to set curproc and curthr. When sched_switch
 * returns, a different thread should be executing than the thread
 * which was executing when sched_switch was called.
 *
 * Note: The IPL is process specific.
 */
void
sched_switch(void)
{
        // NOT_YET_IMPLEMENTED("PROCS: sched_switch");
        
        /* retreive the current interrupt priority level */
        uint8_t old_ipl = intr_getipl();

        /* set IPL to high to prevent interrupts happening at an inpportune moment */
        intr_setipl(IPL_HIGH); // IPL_HIGH is from interrupt.h

        /* have no threads on run queue, so wait for an interrupt */
        while(sched_queue_empty(&kt_runq)){
                /* re-enable interrupts */
                intr_setipl(0);
                /* wait for an interrupt */
                intr_wait();
                /* re-disable interrupts */
                intr_setipl(IPL_HIGH); // IPL_HIGH is from interrupt.h

                dbg(DBG_PRINT, "(GRADING1A)\n");
        }

        /* retrieve the old thread */
        kthread_t* old_thr = curthr;
        /* dequeue a new thread from run queue and assign it to current thread */
        curthr = ktqueue_dequeue(&kt_runq);
        /* set this new thread's process as current process */
        curproc = curthr->kt_proc; //1.0

        /* switch context */
        context_switch(&old_thr->kt_ctx, &curthr->kt_ctx);
        
        /* restore the original IPL */ 
        intr_setipl(old_ipl);

        dbg(DBG_PRINT, "(GRADING1A)\n");
}

/*
 * Since we are modifying the run queue, we _MUST_ set the IPL to high
 * so that no interrupts happen at an inopportune moment.

 * Remember to restore the original IPL before you return from this
 * function. Otherwise, we will not get any interrupts after returning
 * from this function.
 *
 * Using intr_disable/intr_enable would be equally as effective as
 * modifying the IPL in this case. However, in some cases, we may want
 * more fine grained control, making modifying the IPL more
 * suitable. We modify the IPL here for consistency.
 */
void
sched_make_runnable(kthread_t *thr)
{
        // NOT_YET_IMPLEMENTED("PROCS: sched_make_runnable");
        // dbg(DBG_PRINT, "'%s'\n", curproc->p_comm);
        
        /* precondition */
        KASSERT(&kt_runq != thr->kt_wchan);
        dbg(DBG_PRINT, "(GREADING1A 5.a)\n");

        /* retreive the current interrupt priority level */
        uint8_t old_ipl = intr_getipl();
        /* set IPL to high to prevent interrupts happening at an inpportune moment */
        intr_setipl(IPL_HIGH); // IPL_HIGH is from interrupt.h

        /* set state of this thread */
        thr->kt_state = KT_RUN;
        /* remove it from the blocked queue if queue is not NULL */
        if (thr->kt_wchan != NULL){
                ktqueue_remove(thr->kt_wchan, thr);
                dbg(DBG_PRINT, "(GRADING1C)\n");
        }

        KASSERT(NULL != thr);
        // dbg(DBG_PRINT, "The value of kt_runq is: '%p'\n", thr);
        
        /* enqueue this thread onto the run queue */
        ktqueue_enqueue(&kt_runq, thr);

        // dbg(DBG_PRINT, "made\n");

        /* restore the original IPL */
        intr_setipl(old_ipl);

        dbg(DBG_PRINT, "(GRADING1A)\n");
}

