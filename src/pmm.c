#include <print.h>
#include <paging.h>
#include "memory_layout.h"
#include "pmm.h"

uint8 mem_state = 0;
memlist *freelist = 0x0;

struct spin_lock pm_global_lock;

uint64 MEM_LO = 0;
uint64 MEM_HI = 0;
uint64 phase2_offset = 0;

uint8
get_mem_phase()
{
    return mem_state;
}




static uint64 mem_low;
static uint64 mem_high;
static uint64 ptr;

void
pfree2(paddr p)
{
    UNREFERENCED(p);
}

void* pmalloc2(uint32 size)
{
    void* ret = NULL;

    spin_lock(&pm_global_lock);
    if((size > KERNEL_PAGE_SIZE) || (ptr >= mem_high)) {
        ret = NULL;
    } else {
        if ((ptr >= K_IMAGE_PADDR) && (ptr < (K_IMAGE_PADDR + K_IMAGE_PRESRV))) {
            ptr = K_IMAGE_PADDR + K_IMAGE_PRESRV;
        }
        ret = (void *) ptr;
        ptr = ptr + KERNEL_PAGE_SIZE;
    }

    spin_unlock(&pm_global_lock);
    kprintf("Allocated physical: 0x%x, size: %d\n", (uint64)ptr, (uint64)size);
    return ret;
}

void pmm_init2(uint64 low, uint64 high)
{
    ptr = low;
    mem_low = low;
    mem_high = high;
    spin_init(&pm_global_lock);
}


void
pmm_init(uint64 low, uint64 high)
{
    (void)low;

    spin_init(&pm_global_lock);

    mem_state = 1;
    if (mem_state) {
        MEM_LO = low;
        MEM_HI = high; // TODO : TEMPORARY

		// Allocate freelist
        paddr freelist_head = (paddr)pmalloc(PAGE_SIZE);
		freelist = R_PADDR(freelist_head);

        spin_lock(&pm_global_lock);
		lb_llist_init(freelist);

		memnode * freelist_node = (memnode *) (freelist + (sizeof(*freelist)));
		allocu * freelist_unit = (allocu *) (((paddr)freelist_node) + (sizeof(*freelist_node)));
		freelist_unit->size = PAGE_SIZE;
		freelist_unit->status = 1;
		// First allocation unit points to PMM data structure.
		freelist_unit->head = freelist_head;

        freelist_node->data = freelist_unit;

        lb_llist_push_front(freelist, freelist_node);

        mem_state = 2;
    }

    spin_unlock(&pm_global_lock);
}

paddr
freelist_alloc(uint32 n)
{
    memnode *curr_node = freelist->tail;
    while (1) {
        allocu *toalloc = curr_node->data;
        if (toalloc->size >= n && toalloc->status == 0) {
            toalloc->status = 1;
            return toalloc->head;
        }

        if (!curr_node->prev) break;
        curr_node = curr_node->prev;
    }

    memnode * new_node;
    allocu * new_alloc;

    // TODO : Check if there is space for book keeping
    allocu *last_allocu = (allocu *) curr_node->data;
    paddr new_head = ((paddr) last_allocu) + sizeof(allocu);
    new_node = (memnode *) new_head;
    new_alloc = (allocu *) (new_head + sizeof(memnode));

    // Set book keeping info
    new_alloc->size = n;
    new_alloc->status = 1;
    new_alloc->head = last_allocu->head + last_allocu->size;
    
    if (new_alloc->head > (paddr) MEM_HI) {
        *(uint64 *)new_alloc = 0;
        *(uint64 *)new_node = 0;
        return 0;
    }

    usize cross_reserve_adjust = 0;
    if ((new_alloc->head >= K_IMAGE_PADDR) && (new_alloc->head < (K_IMAGE_PADDR + K_IMAGE_PRESRV))) {
        cross_reserve_adjust = K_IMAGE_PADDR + K_IMAGE_PRESRV;
    }

    new_alloc->head += cross_reserve_adjust;
    new_node->data = new_alloc;

    // Push to free list (TODO : TEST)
    lb_llist_push_front(freelist, new_node);

    return new_alloc->head;
}

paddr
alloc_ppage(uint32 n)
{
    paddr allocated = 0;

    if (mem_state == 1) {
        allocated = MEM_LO;
        if (allocated == MEM_HI) return 0;
        phase2_offset += n;
        allocated = (paddr) allocated;
    } else if (mem_state == 2) {
        allocated = freelist_alloc(n);
    }

    return allocated;
}

void
pfree(void * p)
{
    spin_lock(&pm_global_lock);
    memnode *curr_node = freelist->tail;
    while (curr_node) {
        allocu *toalloc = curr_node->data;
        if (toalloc->head == ((paddr) p) && toalloc->status == 1) {
            toalloc->status = 0;
            spin_unlock(&pm_global_lock);
            return;
        }

        curr_node = curr_node->prev;
    }
    spin_unlock(&pm_global_lock);
}

void *
pmalloc(uint32 size)
{
    spin_lock(&pm_global_lock);

    paddr new_alloc_paddr = alloc_ppage(size);

    spin_unlock(&pm_global_lock);

    kprintf("Allocated Physical: 0x%x Size: %x\n", new_alloc_paddr, (uint64)size);
    return (void *) new_alloc_paddr;
}


