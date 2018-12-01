#include "spin_lock.h"
#include "cpu.h"
#include "intr.h"

void
spin_init(struct spin_lock *lock)
{
    if (lock != NULL)
    {
        lock->val = 0;
    }
}


void
spin_lock(struct spin_lock *lock)
{
    if (lock != NULL)
    {
        while (cmpxchg_32(&lock->val, 0, 1) != 0)
        {}
    }
}


void
spin_unlock(struct spin_lock *lock)
{
    if (lock != NULL)
    {
        lock->val = 0;
    }
}

uint64 spin_lock_irq_save(struct spin_lock* lock)
{
    uint64 ret = READ_IRQ();
    WRITE_IRQ(0xf);
    spin_lock(lock);
    return ret;
}

void spin_unlock_irq_restore(struct spin_lock* lock, uint64 irq)
{
    spin_unlock(lock);
    WRITE_IRQ(irq);
}
