#include <print.h>
#include <paging.h>
#include "memory_layout.h"
#include "pmm.h"

uint8 mem_state = 0;
memlist *freelist = 0x0;
uint64 pmem_head = 0;
struct spin_lock pm_global_lock;

uint64 KHEAP_LO = 0;
uint64 KHEAP_HI = 0;

uint64 MEM_LO = 0, LOSZ = 0;
uint64 MEM_HI = 0, HISZ = 0;

void set_mmap_low(uint64 low, uint64 size)
{
    MEM_LO = low;
    LOSZ = size;
}

void set_mmap_high(uint64 high, uint64 size)
{
    MEM_HI = high;
    HISZ = size;

    mem_state = 1; // TODO : Cannot init pmm without full info
}

uint8
get_mem_phase()
{
    return mem_state;
}

void
pmm_init2()
{
    spin_init(&pm_global_lock);

    if (mem_state)
    {
        KHEAP_HI = MEM_HI;
        KHEAP_LO = MEM_HI + HISZ;

		// Freelist allocation hacks
		freelist = pmalloc(4096);

        spin_lock(&pm_global_lock);
		lb_llist_init(freelist);

		memnode * freelist_node = (memnode *) (freelist + (sizeof(*freelist)));
		allocu * freelist_unit = (allocu *) (freelist_node + (sizeof(*freelist_node)));
		freelist_unit->size = 1;
		freelist_unit->status = 1;
		// First allocation unit points to PMM data structure.
		freelist_unit->head = (paddr) freelist;

        freelist_node->data = freelist_unit;

        lb_llist_push_back(freelist, freelist_node);

        // TODO : IF all is good, now we must have a llist of memory
        mem_state = 2;
    }

    spin_unlock(&pm_global_lock);
}

paddr
freelist_alloc(uint32 n)
{
    memnode *curr_node = freelist->head;
    while (1)
    {
        allocu *toalloc = curr_node->data;
        if (toalloc->size >= n && toalloc->status == 0)
        {
            // TODO : Free interval found
            // TODO : NOT ATOMIC
            toalloc->status = 1;
            return toalloc->head;
        }

        if (!curr_node->next)
        { break; }
        curr_node = curr_node->next;
    }

    // TODO : Alloc from front
    allocu *last_alloc = (allocu *) curr_node->data;
    paddr new_head = (paddr) last_alloc + sizeof(*last_alloc);
    memnode *new_node = (memnode *) new_head;
    allocu *new_alloc = (allocu *) (new_head + sizeof(*new_node));

    new_alloc->size = n;
    new_alloc->status = 1;
    new_alloc->head = last_alloc->head - (n * PAGE_SIZE);

    new_node->data = new_alloc;

    lb_llist_push_back(freelist, new_node);

    return new_alloc->head;
}

paddr
alloc_ppage(uint32 n)
{
    paddr allocated;

    if (mem_state == 1)
    {
        uint64 offset = (pmem_head + n) * PAGE_SIZE;
        allocated = KHEAP_LO - offset;
        if (allocated == KHEAP_HI)
        { return 0; }
        pmem_head += n;

        // TODO : TEMP Zero out
        uint64 *zp = (uint64 *) allocated;
        *zp = 0;
    }
    else if (mem_state == 2)
    {
        allocated = freelist_alloc(n);
    }
    else
    {
        // TODO : Here goes the code for pmm after stage 1
        allocated = 0;
    }

    return allocated;
}

void
pfree2(paddr p)
{
    spin_lock(&pm_global_lock);
    memnode *curr_node = freelist->head;
    while (curr_node)
    {
        allocu *toalloc = curr_node->data;
        if (toalloc->head == p && toalloc->status == 1 && toalloc->size > 0)
        {
            toalloc->status = 0;
            spin_unlock(&pm_global_lock);
            return;
        }

        curr_node = curr_node->next;
    }
    spin_unlock(&pm_global_lock);
}

void *
pmalloc2(uint32 size)
{
    spin_lock(&pm_global_lock);
    
    uint32 left_over = size % PAGE_SIZE;
    uint32 num_pages = (size / PAGE_SIZE) + (left_over > 0);
    paddr new_alloc_paddr = alloc_ppage(num_pages);

    spin_unlock(&pm_global_lock);

    kprintf("Allocated Physical: 0x%x Size: %x\n", new_alloc_paddr, (uint64)size);
    return (void *) new_alloc_paddr;
}

static uint64 mem_low;
static uint64 mem_high;
static uint64 ptr;

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
        if ((ptr >= K_IMAGE_PADDR) && (ptr < (K_IMAGE_PADDR + K_IMAGE_PRESRV)))
        {
            ptr = K_IMAGE_PADDR + K_IMAGE_PRESRV;
        }

        ret = (void *) ptr;

        ptr = ptr + KERNEL_PAGE_SIZE;
    }

    spin_unlock(&pm_global_lock);

    kprintf("Allocated physical: 0x%x, size: %d\n", (uint64)ptr, (uint64)size);

    return ret;
}

void pmm_init(uint64 low, uint64 high)
{
    ptr = low;
    mem_low = low;
    mem_high = high;
    spin_init(&pm_global_lock);
}
