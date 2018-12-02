#pragma once

#include "proc.h"

struct tcb
{
    struct pcb *proc;
    uint32 tid;
    int32 exit_code;
    uint32 state;

    void* stack0_top;
    uint64 stack0_size;

    uint64 rsp0; // kernel stack pointer for the stack, has context information
    struct spin_lock lock;
    struct llist_node list_node;
};

struct tcb* get_cur_thread();

int32 thread_stop(uint32 tid, int32 code);

void list_threads();

int32 thread_create(struct pcb* proc, void (*entry)(void*), void* args, uint32* tid);

void thread_init();

void thread_schedule();

int32 thread_get_exit_code(uint32 tid, int32* exit_code);

void thread_yield();

int32 thread_resume(uint32 tid);

int32 thread_block(uint32 tid);
