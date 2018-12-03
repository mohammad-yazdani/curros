#include <print.h>
#include "thread.h"
#include "error.h"
#include "proc.h"
#include "clib.h"
#include "vmm.h"
#include "cpu.h"
#include "intr.h"
#include "memory_layout.h"

#define THREAD_STACK_SIZE (PAGE_SIZE)

enum
{
    THREAD_STATE_RDY = 0,
    THREAD_STATE_BLK = 1,
    THREAD_STATE_ZOM = 2,
};

static uint32 thread_id;
static struct tcb *cur_thread;
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
        kprintf("Thread %d\n", (uint64) tcb->tid);
        node = lb_llist_next(node);
    }
}

static struct tcb *get_tcb_by_id(uint32 tid)
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
            thread_yield();
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
        thread_yield();
    }

    return ret;
}

void set_cur_thread(struct tcb *t)
{
    // first time setting
    KASSERT(cur_thread == NULL);
    cur_thread = t;
}

// this guy picks the next thread to run
// by updating cur_thread
// timer interrupt always
// the actual context swap is done elsewhere
void thread_schedule()
{
    // only timer interrupt context
    spin_lock(&thread_list_lock);

    if (cur_thread == NULL)
    {
        // first thread hack
        cur_thread = OBTAIN_STRUCT_ADDR(lb_llist_first(&thread_list), struct tcb, list_node);
    }
    KASSERT(cur_thread != NULL);

    struct llist_node *node = lb_llist_next(&cur_thread->list_node);

    if (node == NULL)
    {
        node = lb_llist_first(&thread_list);
    }

    // since there must be a null thread, node cannot be null
    KASSERT(node != NULL);
    while (node != NULL)
    {
        struct tcb *next = OBTAIN_STRUCT_ADDR(node, struct tcb, list_node);

        // we are only checking states here so no need to worry
        // since thread_block always fires an timer interrupt, which will be served immediately after this returns
        // so even if someone else from another core blocks this thread during this window
        // this thread will get descheduled right after the current timer interrupt deasserts
        // the worst case is someone unblocked a thread right after we checked it, but it doesn't break integrity
        if (next->state == THREAD_STATE_RDY && next != cur_thread)
        {
            cur_thread = next;
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

void thread_exit(int32 code)
{
    spin_lock(&cur_thread->lock);
    KASSERT(cur_thread->state == THREAD_STATE_RDY);
    cur_thread->exit_code = code;
    cur_thread->state = THREAD_STATE_ZOM;
    spin_unlock(&cur_thread->lock);
    thread_yield();
    // shouldn't get here
    KASSERT(FALSE);
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
        thread_yield();
    }

    return ret;
}

void thread_yield()
{
    // emulate a timer interrupt on target core
    send_ipi(INTR_VEC_TIMER);
}

#define MODE_K (0)
#define MODE_U (1)

static void write_intr_frame(struct intr_frame *frame, uint32 mode, uint64 iret_addr, uint64 iret_stack, uint64 arg)
{
    uint64 dsel = mode == MODE_K ? SEL(GDT_K_DATA, 0, 0) : SEL(GDT_U_DATA, 0, 3);
    uint64 csel = mode == MODE_K ? SEL(GDT_K_CODE, 0, 0) : SEL(GDT_U_CODE, 0, 3);
    frame->ss = dsel;
    frame->ds = dsel;
    frame->es = dsel;
    frame->fs = dsel;
    frame->gs = dsel;
    frame->cs = csel;
    frame->rip = (uint64) iret_addr;
    frame->rdi = arg;
    frame->rsp = iret_stack;

    // only set interrupt enable flag
    frame->rflags = (0x200);
}

int32 thread_create(struct pcb *proc, void (*func)(void *), void *args, uint32 *tid)
{
    int32 ret = ESUCCESS;

    struct tcb *tcb = kalloc(sizeof(struct tcb));

    if (tcb == NULL)
    {
        ret = ENOMEM;
    }

    // allocate thread kernel stack
    // 4k stack for now
    // no stack overflow
    uint64 *kstack_top = NULL;
    if (ret == ESUCCESS)
    {
        kstack_top = kalloc(THREAD_STACK_SIZE);

        if (kstack_top == NULL)
        {
            ret = ENOMEM;
        }
        else
        {
            kstack_top = (uint64 *) ((uintptr) kstack_top + THREAD_STACK_SIZE);
        }
    }

    if (ret == ESUCCESS)
    {
        tcb->tid = (uint32) xinc_32((int32 *) &thread_id, 1);
        spin_init(&tcb->lock);
        tcb->exit_code = 0;
        tcb->state = THREAD_STATE_RDY;
        tcb->proc = proc;
        tcb->stack0_top = kstack_top;
        tcb->stack0_size = THREAD_STACK_SIZE;

        // write initial context information on the kernel stack
        struct intr_frame *frame = (struct intr_frame *) ((uintptr) kstack_top - sizeof(struct intr_frame));
        mem_set(frame, 0, sizeof(struct intr_frame));

        // here we wanna check if the entrance is user mode
        if (IS_KERN_SPACE(func))
        {
            write_intr_frame(frame, MODE_K, (uint64) func, (uint64) kstack_top, (uint64) args);
        }
        else
        {
            KASSERT(FALSE);
            write_intr_frame(frame, MODE_U, (uint64) func, (uint64) NULL, (uint64) args);
        }

        // update interrupt stack pointer
        tcb->rsp0 = (uint64) frame;

        // add to thread list
        uint64 irq;
        irq = spin_lock_irq_save(&thread_list_lock);
        lb_llist_push_back(&thread_list, &tcb->list_node);
        spin_unlock_irq_restore(&thread_list_lock, irq);
        *tid = tcb->tid;
    }

    return ret;
}

void thread_init()
{
    cur_thread = NULL;
    lb_llist_init(&thread_list);
    spin_init(&thread_list_lock);
    thread_id = 0;
}

struct tcb *get_cur_thread()
{
    return cur_thread;
}
