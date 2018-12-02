#pragma once

#include "proc.h"

struct tcb
{
    struct pcb *proc;
    uint32 tid;
    uint32 core_id;
    int32 exit_code;
    uint32 state;

    // things that are allocated in this tcb
    void* stack0;

    uint64 rsp0; //kernel stack for the thread, context information is store on the kernel stack
    struct spin_lock lock;
    struct llist_node list_node;
};

struct tcb* get_cur_thread();

void thread_exit(int32 code);

void list_threads();

int32 thread_create(struct pcb* proc, void (*entry)(void*), void* args, uint32* tid);

void thread_init();

void thread_schedule();

int32 thread_get_exit_code(uint32 tid, int32* exit_code);

int32 thread_stop(uint32 tid, int32 code);

void thread_yield(uint32 core_id);

int32 thread_resume(uint32 tid);

int32 thread_block(uint32 tid);
