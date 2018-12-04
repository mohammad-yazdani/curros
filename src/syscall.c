#include <print.h>
#include "syscall.h"
#include "thread.h"
#include "intr.h"

void *syscall_handler(struct intr_frame *frame)
{
    uint64 vec = (uint32)frame->rdi;
    if (vec == 0)
    {
        // print
        kprintf("%s", (char*)frame->rsi);
    }
    else
    {
        kprintf("[WARN] Unrecognized syscall from proc %d, thread %d, function %d\n", (uint64) get_cur_thread()->proc->proc_id,
                (uint64) get_cur_thread()->tid,
                vec);
    }

    // nothing for the handler
    return NULL;
}

void syscall_init()
{
    set_intr_handler(INTR_VEC_SYSCALL, syscall_handler);
}
