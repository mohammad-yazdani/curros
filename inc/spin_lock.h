#pragma once

#include "cdef.h"

struct spin_lock
{
    int32 val;
};

void spin_init(struct spin_lock *lock);

void spin_lock(struct spin_lock *lock);

void spin_unlock(struct spin_lock *lock);

uint64 spin_lock_irq_save(struct spin_lock* lock);

void spin_unlock_irq_restore(struct spin_lock* lock, uint64 irq);