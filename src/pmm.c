#include <print.h>
#include <paging.h>
#include "memory_layout.h"
#include "pmm.h"
#include "clib.h"

static uint64 mem_low;
static uint64 mem_high;
static uint64 ptr;
static struct spin_lock pm_global_lock;

void
pfree(paddr p)
{
    UNREFERENCED(p);
}

void* pmalloc(uint32 size)
{
    void* ret = NULL;

    spin_lock(&pm_global_lock);
    if((size > KERNEL_PAGE_SIZE) || (ptr >= mem_high))
    {
        ret = NULL;
    }
    else
    {
        ret = (void *) ptr;
        ptr = ptr + KERNEL_PAGE_SIZE;
    }

    spin_unlock(&pm_global_lock);

#ifdef KDBG
    kprintf("Allocated physical: 0x%x, size: %d\n", (uint64)ptr, (uint64)size);
#endif

    return ret;
}

void pmm_init(uint64 low, uint64 high)
{
    mem_low = MAX(K_IMAGE_PADDR + K_IMAGE_PRESRV, low);
    ptr = mem_low;
    mem_high = high;
    spin_init(&pm_global_lock);
}
