#pragma once

#include "cdef.h"
#include "llist.h"
#include "spin_lock.h"

struct pcb
{
    uint64 cr3;
    uint32 proc_id;
    struct llist threads;
    struct llist_node list_node;
    struct spin_lock lock;
};

// procs now are simply cr3 holders

int32 proc_create(void (*func)(void*), uint32* proc_id);

void proc_init();
