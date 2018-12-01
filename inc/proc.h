#pragma once

#include "cdef.h"
#include "llist.h"
#include "spin_lock.h"

struct pcb
{
    uint64 cr3;
    uint32 proc_id;
    struct llist threads;
    struct spin_lock lock;
};

int32 process_create(void* entry, uint32* proc_id);