#include <cdef.h>
#include <vmm.h>
#include <pmm.h>
#include <paging.h>
#include <memory_layout.h>
#include <error.h>
#include <cpu.h>
#include <print.h>

static void *pointer;
static struct spin_lock lock;

void *kalloc(usize size)
{
    spin_lock(&lock);

    void *ret;
    if ((size > KERNEL_PAGE_SIZE) || ((uintptr) pointer + size >= K_DYN_END))
    {
        ret = NULL;
    }
    else
    {
        ret = pointer;
        pointer = (void *) ((uintptr) pointer + KERNEL_PAGE_SIZE);
    }

    spin_unlock(&lock);

    /* Hack */
    int32 status = ESUCCESS;
    if (ret != NULL)
    {
        // check if the target page is already mapped
        // ret val does not cross page boundries
        if (get_paddr(read_cr3(), (uintptr) ret) == (uintptr) NULL)
        {
            uintptr frame = (uintptr) pmalloc(PAGE_SIZE);
            status = (frame != (uintptr) NULL) && (map_vmem(read_cr3(), (uintptr) ret, frame) != ESUCCESS);
        }
    }

    if (status != ESUCCESS)
    {
        ret = NULL;
    }
    else
    {
        flush_tlb();
    }

#ifdef KDBG
    kprintf("Allocated Virtual: 0x%x Size: %d\n", ret, (uint64) size);
#endif

    return ret;
}

void init_vm()
{
    pointer = (void *) K_DYNAMIC;
    spin_init(&lock);
}

void
kfree(void *ptr)
{
    UNREFERENCED(ptr);
}
