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
int32 proc_create(void* elf64, uint32* proc_id);


// proc init also makes the current address space process 0
// and creates a thread to run k_routine
int32 proc_init(void* k_routine);
