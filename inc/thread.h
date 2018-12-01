#pragma once

#include "proc.h"

struct tcb
{
    struct proc *proc;
    uint32 tid;
    int exit_code;
    uint32 state;
    uint64 rsp0; //kernel stack for the thread, context information is store on the kernel stack
    struct spin_lock lock;
};

struct tcb* get_cur_thread();

void thread_exit(int32 code);

int32 thread_create(void (*func)(void*), void* args, uint32* tid);

int32 thread_init();
