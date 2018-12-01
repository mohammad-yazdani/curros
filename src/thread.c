#include "thread.h"
#include "error.h"
#include "proc.h"
#include "alloc.h"
#include "cpu.h"

enum
{
    THREAD_STATE_RDY = 0,
    THREAD_STATE_BLK = 1,
    THREAD_STATE_DED = 2,
};

static uint32 thread_id;
static struct tcb *cur_thread;
static struct llist thread_list;

void thread_exit(int32 code)
{

}

int32 thread_create(void (*func)(void *), void *args, uint32 *tid)
{
    struct tcb *tcb = kalloc(sizeof(struct tcb));

    if (tcb == NULL)
    {
        return ENOMEM;
    }

    tcb->tid = (uint32) xinc_32((int32 *) &thread_id, 1);
    spin_init(&tcb->lock);
    tcb->exit_code = 0;
    tcb->state = THREAD_STATE_RDY;
    tcb->proc = cur_thread->proc;
    tcb->rsp0 = ;
}

int32 thread_init()
{
    next_thread = NULL;
    cur_thread = NULL;
}

struct tcb *get_cur_thread()
{
    return cur_thread;
}