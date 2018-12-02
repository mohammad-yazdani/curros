#include "thread.h"
#include "error.h"
#include "proc.h"
#include "vmm.h"
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
    UNREFERENCED(code);
}

int32 thread_create(void (*func)(void *), void *args, uint32 *tid)
{
    UNREFERENCED(func);
    UNREFERENCED(args);
    UNREFERENCED(tid);

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

    return ENOMEM;
}

int32 thread_init()
{
    cur_thread = NULL;
    lb_llist_init(&thread_list);
    return ENOMEM;
}

struct tcb *get_cur_thread()
{
    return cur_thread;
}
