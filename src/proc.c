#include "error.h"
#include "proc.h"
#include "thread.h"

static struct llist proc_list;
static uint32 tid;

int32 process_init()
{
    lb_llist_init(&proc_list);
    tid = 0;
    return ESUCCESS;
}

int32 process_create(void *entry, uint32 *proc_id)
{
    // allocate pcb and stuff
    return ESUCCESS;
}