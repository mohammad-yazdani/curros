#include <print.h>
#include "thread.h"
#include "error.h"
#include "proc.h"
#include "clib.h"
#include "vmm.h"
#include "cpu.h"
#include "intr.h"

#define THREAD_STACK_SIZE (2048)

enum
{
    THREAD_STATE_RDY = 0,
    THREAD_STATE_BLK = 1,
    THREAD_STATE_ZOM = 2,
};

static uint32 thread_id;
static struct tcb *cur_thread[NUM_CORES];
static struct llist thread_list;
static struct spin_lock thread_list_lock;

void list_threads()
{
    struct llist_node *node = lb_llist_first(&thread_list);
    struct tcb *tcb = NULL;
    while (node != NULL)
    {
        tcb = OBTAIN_STRUCT_ADDR(node, struct tcb, list_node);
        // reading no need to lock thread lock
        kprintf("Thread - %d\n", (uint64)tcb->tid);
        node = lb_llist_next(node);
    }

    return;
}

struct tcb *get_tcb_by_id(uint32 tid)
{
    // the thread list lock must be held
    struct llist_node *node = lb_llist_first(&thread_list);
    struct tcb *tcb = NULL;
    while (node != NULL)
    {
        tcb = OBTAIN_STRUCT_ADDR(node, struct tcb, list_node);
        // reading no need to lock thread lock
        if (tcb->tid == tid)
        {
            break;
        }
        node = lb_llist_next(node);
    }

    return tcb;
}

int32 thread_get_exit_code(uint32 tid, int32 *exit_code)
{
    int32 ret = ESUCCESS;

    struct tcb *tcb;
    uint64 irq = spin_lock_irq_save(&thread_list_lock);
    tcb = get_tcb_by_id(tid);
    spin_unlock_irq_restore(&thread_list_lock, irq);

    if (tcb == NULL)
    {
        ret = EINVARG;
    }

    if (ret == ESUCCESS)
    {
        if (tcb->state == THREAD_STATE_ZOM)
        {
            *exit_code = tcb->exit_code;

            // this is unfair but do it for now
            thread_yield(tcb->core_id);
        }
        else
        {
            ret = EINVARG;
        }
    }

    return ret;
}

int32 thread_stop(uint32 tid, int32 code)
{
    int32 ret = ESUCCESS;

    struct tcb *tcb;
    uint64 irq = spin_lock_irq_save(&thread_list_lock);
    tcb = get_tcb_by_id(tid);
    spin_unlock_irq_restore(&thread_list_lock, irq);

    if (tcb == NULL)
    {
        ret = EINVARG;
    }

    if (ret == ESUCCESS)
    {
        spin_lock(&tcb->lock);
        if (tcb->state != THREAD_STATE_ZOM)
        {
            tcb->state = THREAD_STATE_ZOM;
            tcb->exit_code = code;
        }
        else
        {
            ret = EINVARG;
        }
        spin_unlock(&tcb->lock);
    }

    if (ret == ESUCCESS)
    {
        // well this is unfair
        // but do this for now
        thread_yield(tcb->core_id);
    }

    return ret;
}

// this guy picks the next thread to run
// by updating cur_thread
// timer interrupt always
// the actual context swap is done elsewhere
void thread_schedule()
{
    // only timer interrupt context
    uint32 core_id = get_core();
    spin_lock(&thread_list_lock);

    struct llist_node *node = lb_llist_first(&thread_list);

    // since there must be a null thread, node cannot be null
    KASSERT(node != NULL);
    while (node != NULL)
    {
        struct tcb *next = OBTAIN_STRUCT_ADDR(node, struct tcb, list_node);

        if (next->core_id != core_id)
        {
            // not for the current core
            continue;
        }

        // we are only checking states here so no need to worry
        // since thread_block always fires an timer interrupt, which will be served immediately after this returns
        // so even if someone else from another core blocks this thread during this window
        // this thread will get descheduled right after the current timer interrupt deasserts
        // the worst case is someone unblocked a thread right after we checked it, but it doesn't break integrity
        if (next->state == THREAD_STATE_RDY && next != cur_thread[core_id])
        {
            cur_thread[core_id] = next;
            // stop looping, scheduler runs fast, although o(n)
            break;
        }
        else if (next->state == THREAD_STATE_ZOM)
        {
            // zombie thread, should use ref count to properly deallocate stuff
            // TODO: clean up zombie threads
        }

        node = lb_llist_next(node);
    }

    spin_unlock(&thread_list_lock);
}


int32 thread_resume(uint32 tid)
{
    int32 ret = ESUCCESS;

    struct tcb *tcb;
    uint64 irq = spin_lock_irq_save(&thread_list_lock);
    tcb = get_tcb_by_id(tid);
    spin_unlock_irq_restore(&thread_list_lock, irq);

    if (tcb == NULL)
    {
        ret = EINVARG;
    }

    if (ret == ESUCCESS)
    {
        spin_lock(&tcb->lock);
        if (tcb->state == THREAD_STATE_BLK)
        {
            tcb->state = THREAD_STATE_RDY;
        }
        else
        {
            ret = EINVARG;
        }
        spin_unlock(&tcb->lock);
    }

    return ret;
}

int32 thread_block(uint32 tid)
{
    int32 ret = ESUCCESS;

    struct tcb *tcb;
    uint64 irq = spin_lock_irq_save(&thread_list_lock);
    tcb = get_tcb_by_id(tid);
    spin_unlock_irq_restore(&thread_list_lock, irq);

    if (tcb == NULL)
    {
        ret = EINVARG;
    }

    if (ret == ESUCCESS)
    {
        spin_lock(&tcb->lock);
        if (tcb->state == THREAD_STATE_RDY)
        {
            tcb->state = THREAD_STATE_BLK;
        }
        else
        {
            ret = EINVARG;
        }
        spin_unlock(&tcb->lock);
    }

    if (ret == ESUCCESS)
    {
        // this is unfair but do it for now
        thread_yield(tcb->core_id);
    }

    return ret;
}

void thread_yield(uint32 core_id)
{
    // emulate a timer interrupt on target core
    send_ipi(core_id, INTR_VEC_TIMER);
}


int32 thread_create(struct pcb *proc, void (*func)(void *), void *args, uint32 *tid)
{
    int32 ret = ESUCCESS;

    struct tcb *tcb = kalloc(sizeof(struct tcb));

    if (tcb == NULL)
    {
        ret = ENOMEM;
    }

    kprintf("tcb: 0x%x\n", tcb);

    // allocate thread kernel stack
    // 4k stack for now
    // no stack overflow
    uint64 *kstack = NULL;
    if (ret == ESUCCESS)
    {
        kstack = kalloc(THREAD_STACK_SIZE);

        if (kstack == NULL)
        {
            ret = ENOMEM;
        }
        else
        {
            kstack = (uint64 *) ((uintptr) kstack + THREAD_STACK_SIZE);
        }
    }

    if (ret == ESUCCESS)
    {
        tcb->tid = (uint32) xinc_32((int32 *) &thread_id, 1);
        spin_init(&tcb->lock);
        tcb->exit_code = 0;
        tcb->state = THREAD_STATE_RDY;
        tcb->proc = proc;
        tcb->stack0 = kstack;
        tcb->core_id = get_core();

        // write initial context information on the kernel stack
        struct intr_frame *frame = (struct intr_frame *) ((uintptr) kstack - sizeof(struct intr_frame));
        mem_set(frame, 0, sizeof(struct intr_frame));

        // here we wanna check if the entrance is user mode
        if (IS_KERN_SPACE(func))
        {
            // write seg registers
            frame->ss = SEL(GDT_K_DATA, 0, 0);
            frame->ds = SEL(GDT_K_DATA, 0, 0);
            frame->es = SEL(GDT_K_DATA, 0, 0);
            frame->fs = SEL(GDT_K_DATA, 0, 0);
            frame->gs = SEL(GDT_K_DATA, 0, 0);
            frame->cs = SEL(GDT_K_CODE, 0, 0);

            // write parameters
            frame->rdi = (uint64) args;
            // let it use kstack after it pops iretq stuff out
            frame->rsp = (uint64) kstack;

            // write return info
            // only set interrupt enable flag
            frame->rflags = (0x200);
            frame->rip = (uint64) func;
        }
        else
        {
            // write seg registers
            frame->ss = SEL(GDT_U_DATA, 0, 3);
            frame->ds = SEL(GDT_U_DATA, 0, 3);
            frame->es = SEL(GDT_U_DATA, 0, 3);
            frame->fs = SEL(GDT_U_DATA, 0, 3);
            frame->gs = SEL(GDT_U_DATA, 0, 3);
            frame->cs = SEL(GDT_U_CODE, 0, 3);

            // write parameters
            frame->rdi = (uint64) args;

            // write return info
            // only set interrupt enable flag
            frame->rflags = (0x200);
            frame->rip = (uint64) func;
        }

        // update kernel stack pointer
        tcb->rsp0 = (uint64) frame;

        // add to thread list
        uint64 irq;
        list_threads();
        irq = spin_lock_irq_save(&thread_list_lock);
        lb_llist_push_back(&thread_list, &tcb->list_node);
        spin_unlock_irq_restore(&thread_list_lock, irq);

        *tid = tcb->tid;
    }

    return ret;
}

void thread_init()
{
    for (int i = 0; i < NUM_CORES; i++)
    {
        cur_thread[i] = NULL;
    }

    lb_llist_init(&thread_list);
    spin_init(&thread_list_lock);
    thread_id = 0;
}

struct tcb *get_cur_thread()
{
    return cur_thread[get_core()];
}
