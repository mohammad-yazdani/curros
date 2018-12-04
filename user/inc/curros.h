#pragma once

#include "cdef.h"

#define SYSCALL_FUNC_PRINT (0)

void syscall(uint32 func, void* args);

static inline void poor_sleep(uint32 dat)
{
    for(uint32 i = 0; i < dat; i++)
    {
    }
}
