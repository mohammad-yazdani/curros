#include <cdef.h>
#include <vmm.h>
#include <pmm.h>
#include "paging.h"
#include <memory_layout.h>
#include <error.h>
#include <cpu.h>
#include "print.h"

#define SYS_TOTAL (K_DYN_END - K_DYNAMIC) // heap start - end
#define TOTAL_PAGE_NUM (SYS_TOTAL / PAGE_SIZE)

#define BKTSZ 32

#define SECTOR0_BOUND 64
#define SECTOR1_BOUND 512

vmem *kmem = 0x0;
vm_node *global_list = 0x0;

vm_node *
get_alloc_node()
{
    usize allocsz = sizeof(vm_node) + sizeof(vm_atom);
    vm_node *alloc = NULL;
    vm_atom *atom = NULL;

    if (kmem->global_alloc->size < 320)
    { // TODO : Temporary hard code
        vaddr tail = (vaddr) kmem->global_alloc->tail;
        if (!tail)
        { tail = (vaddr) kmem->global_alloc; }
        alloc = (void *) (tail + allocsz);
        atom = (void *) ((vaddr) alloc + sizeof(vm_node));
        alloc->data = atom;
        lb_llist_push_back(kmem->global_alloc, alloc);
    }
    else
    {
        alloc = kalloc(sizeof(vm_node));
        atom = kalloc(sizeof(vm_atom));
        alloc->data = atom;
    }

    return alloc;
}

void *
vm_alloc(usize size, uint8 sector_id)
{
    vm_sector *sector = &(kmem->sectors[sector_id]);
    if (size < sector->largest_free->free)
    {
        for (uint32 i = 0; i < BKTSZ; i++)
        {
            vm_object *obj = sector->actbck[i];
            if (size < obj->free)
            {
                // Allocate
                vaddr offset = 0;
                if (obj->allocs.tail)
                {
                    vm_node *tail_node = obj->allocs.tail;
                    vm_atom *tail_atom = tail_node->data;
                    offset += tail_atom->offset + tail_atom->size;
                }

                vm_node *node = get_alloc_node();
                vm_atom *atom = node->data;

                atom->size = size;
                atom->page_ptr = obj;
                atom->offset = offset;

                lb_llist_push_back(&(obj->allocs), node);

                obj->free -= size;
                if (!obj->status)
                {
                    obj->status = 1;

                    if (sector->dirty == 31)
                    {
                        sector->largest_free = sector->actbck[0];
                        for (uint32 j = 1; j < 32; j++)
                        {
                            if (sector->actbck[j]->free > sector->largest_free->free)
                            {
                                sector->largest_free = sector->actbck[j];
                            }
                        }
                    }
                    sector->dirty += 1;
                }
                if (sector->dirty == 32)
                {
                    if (sector->largest_free->free < obj->free)
                    {
                        sector->largest_free = obj;
                    }
                }

                void *ptr = (void *) (atom->offset + obj->address);

                return ptr;
            }
        }
        return 0;
    }
    else
    {
        // TODO : Swap active and reserve
        // TODO : Re-fill reserve
        return 0;
    }
}

/* Subsystem interface */

void
init_vm()
{
    kmem = pmalloc(PAGE_SIZE);
    kmem->groups = 0x0; // TODO : This is for address spaces

    kmem->global_alloc = pmalloc(4 * PAGE_SIZE);
    kmem->global_alloc->head = 0x0;
    kmem->global_alloc->tail = 0x0;
    kmem->global_alloc->size = 0;

    // Set up pages
    usize vm_pages_books = (TOTAL_PAGE_NUM / sizeof(vm_object));
    kmem->pages = pmalloc(vm_pages_books * PAGE_SIZE);
    vaddr curr_address = K_DYNAMIC;

    uint64 counter = 0;
    while (curr_address < K_DYN_END)
    {
//        kprintf("kmem: 0x%x, kmem->pages: 0x%x counter: %d\n", kmem, kmem->pages, counter);
        vm_object *obj = &(kmem->pages[counter]);
        obj->address = curr_address;
        lb_llist_init(&(obj->allocs));
        obj->table_ptr = 0x0;
        obj->status = 0;
        obj->free = PAGE_SIZE;

        curr_address += PAGE_SIZE;
        counter += 1;
    }

    // Set up sectors
    vm_sector *sector0, *sector1, *sector2;

    sector0 = &(kmem->sectors[0]);
    sector0->start = 0;
    sector0->end = TOTAL_PAGE_NUM / 4;
    sector0->largest_free = 0x0;

    sector1 = &(kmem->sectors[1]);
    sector1->start = sector0->end;
    sector1->end = sector1->start + (TOTAL_PAGE_NUM / 2);
    sector1->largest_free = 0x0;

    sector2 = &(kmem->sectors[2]);
    sector2->start = sector1->end;
    sector2->end = TOTAL_PAGE_NUM;
    sector2->largest_free = 0x0;

    for (uint8 secnum = 0; secnum < 3; secnum++)
    {
        vm_sector *sector = &(kmem->sectors[secnum]);

        sector->actbck = pmalloc(PAGE_SIZE);
        sector->resbck = pmalloc(PAGE_SIZE);
        for (uint32 i = 0; i < BKTSZ; i++)
        {
            vm_object *obj_act = &(kmem->pages[sector->start + i]);
            vm_object *obj_res = &(kmem->pages[sector->start + i + BKTSZ]);
            sector->actbck[i] = obj_act;
            sector->resbck[i] = obj_res;
        }

        sector->largest_free = sector->actbck[0];
        sector->dirty = 0;
    }
}

void
vm_issue_unit(uint64 id, usize size)
{
    UNREFERENCED(id);
    UNREFERENCED(size);
}

void *
kalloc(usize size)
{
    uint8 sector_id;
    if (size <= SECTOR0_BOUND)
    {
        sector_id = 0;
    }
    else if (size <= SECTOR1_BOUND)
    {
        sector_id = 1;
    }
    else if (size <= PAGE_SIZE)
    {
        sector_id = 2;
    }
    else
    {
        // TODO : Handle large alloc
        return NULL;
    }

    void *ret = vm_alloc(size, sector_id);

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
        kfree(ret);
        ret = NULL;
    }
    else
    {
        flush_tlb();
    }

    return ret;
}

void
kfree(void *ptr)
{
    UNREFERENCED(ptr);
}

