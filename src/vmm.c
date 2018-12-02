#include <cdef.h>
#include <vmm.h>
#include <pmm.h>
#include <paging.h>
#include <memory_layout.h>
#include <error.h>
#include <cpu.h>
#include <print.h>

#define SYS_TOTAL (K_DYN_END - K_DYNAMIC) // heap start - end
#define TOTAL_PAGE_NUM (SYS_TOTAL / PAGE_SIZE)

#define BKTSZ 32

#define SECTOR0_BOUND 64
#define SECTOR1_BOUND 512

vmem * kmem = 0x0;
vm_node * global_list = 0x0;
struct spin_lock * vm_global_lock = NULL;

paddr
get_vaddr(vaddr v)
{
    paddr p = v - K_IMAGE;
    return p;
}

uint64
get_index(vaddr address)
{
    vaddr offset = address - K_DYNAMIC;
    return (offset / PAGE_SIZE);
}

vm_node *
get_alloc_node()
{
    usize allocsz = sizeof(vm_node) + sizeof(vm_atom);
    vm_node *alloc = NULL;
    vm_atom *atom = NULL;

    if (kmem->global_alloc->size < 320) {
        vaddr tail = (vaddr) kmem->global_alloc->tail;
        if (!tail)
        { tail = (vaddr) kmem->global_alloc; }
        alloc = (void *) (tail + allocsz);
        atom = (void *) ((vaddr) alloc + sizeof(vm_node));
        alloc->data = atom;
        lb_llist_push_back(kmem->global_alloc, alloc);
    } else {
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
    if (size < sector->largest_free->free) {
        for (uint32 i = 0; i < BKTSZ; i++) {
            vm_object *obj = sector->actbck[i];
            if (size < obj->free) {
                // Allocate
                vaddr offset = 0;
                if (obj->allocs.head) {
                    vm_node *head_node = obj->allocs.head;
                    vm_atom *head_atom = head_node->data;
                    offset += head_atom->offset + head_atom->size;
                }

                vm_node *node = get_alloc_node();
                vm_atom *atom = node->data;

                atom->size = size;
                atom->page_ptr = obj;
                atom->offset = offset;

                lb_llist_push_front(&(obj->allocs), node);

                obj->free -= size;
                if (obj->status == 2) {
                    obj->status = 1;

                    if (sector->dirty == 31) {
                        sector->largest_free = sector->actbck[0];
                        for (uint32 j = 1; j < 32; j++) {
                            if (sector->actbck[j]->free > sector->largest_free->free) {
                                sector->largest_free = sector->actbck[j];
                            }
                        }
                    }
                    sector->dirty += 1;
                }
                if (sector->dirty == 32) {
                    if (sector->largest_free->free < obj->free) {
                        sector->largest_free = obj;
                    }
                }

                void *ptr = (void *) (atom->offset + obj->address);

                return ptr;
            }
        }
        return 0;
    } else {
        vm_object ** old = sector->actbck;
        
        sector->actbck = sector->resbck;
        sector->largest_free = sector->actbck[0];
        sector->dirty = 0;

        uint32 i = 0;
        uint64 limit = sector->start;
        while (i < BKTSZ && limit < sector->end) {
            vm_object * page = &(kmem->pages[limit]);
            if (page->status == 0) {
                page->status = 2;
                old[i] = page;
                i += 1;
            }
            limit += 1;
        }

        if (i < 30) {
            kprintf("WARNING: NOT ENOUGH MEM TO FILL BUCKET.\n");
        }
        sector->resbck = old;

        return 0;
    }
}

/* Subsystem interface */

void
init_vm(struct spin_lock * vmlk)
{
    vm_global_lock = vmlk;

    spin_init(vm_global_lock);
    
    spin_lock(vm_global_lock);

    kmem = pmalloc(PAGE_SIZE);
    kmem->groups = 0x0;

    kmem->global_alloc = pmalloc(4 * PAGE_SIZE);
    kmem->global_alloc->head = 0x0;
    kmem->global_alloc->tail = 0x0;
    kmem->global_alloc->size = 0;

    // Set up pages
    usize vm_pages_books = (TOTAL_PAGE_NUM / sizeof(vm_object));
    kmem->pages = pmalloc(vm_pages_books * PAGE_SIZE);
    vaddr curr_address = K_DYNAMIC;

    uint64 counter = 0;
    while (curr_address < K_DYN_END) {
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
    sector1->end = sector1->start + (TOTAL_PAGE_NUM / 4);
    sector1->largest_free = 0x0;

    sector2 = &(kmem->sectors[2]);
    sector2->start = sector1->end;
    sector2->end = sector2->start + (TOTAL_PAGE_NUM / 4);
    sector2->largest_free = 0x0;

    kmem->free_frames = TOTAL_PAGE_NUM / 4;
    kmem->pages[sector2->end].free = kmem->free_frames;

    for (uint8 secnum = 0; secnum < 3; secnum++) {
        vm_sector *sector = &(kmem->sectors[secnum]);

        sector->actbck = pmalloc(PAGE_SIZE);
        sector->resbck = pmalloc(PAGE_SIZE);
        for (uint32 i = 0; i < BKTSZ; i++) {
            vm_object *obj_act = &(kmem->pages[sector->start + i]);
            vm_object *obj_res = &(kmem->pages[sector->start + i + BKTSZ]);

            obj_act->status = 2;
            obj_res->status = 2;

            sector->actbck[i] = obj_act;
            sector->resbck[i] = obj_res;
        }

        sector->largest_free = sector->actbck[0];
        sector->dirty = 0;
    }

    spin_unlock(vm_global_lock);
}

void
vm_issue_unit(uint64 id, usize size)
{
    UNREFERENCED(id);
    UNREFERENCED(size);
}

void *
vm_alloc_multpages(usize size)
{
    usize left_over = size % PAGE_SIZE;
    uint64 n = (size / PAGE_SIZE) + 1;

    uint64 head = kmem->sectors[2].end;
    vm_object *frame = NULL;
    vm_object * end_frame = NULL;

    for (uint64 i = head; i < TOTAL_PAGE_NUM; i++) {
        frame = &(kmem->pages[i]);
        uint64 mem_free = (frame->free / PAGE_SIZE) + 1;
        if (frame->status == 0) {
            if (mem_free >= n) {
                frame->status = 3;
                frame->free = size;
                kmem->free_frames -= n;
                end_frame = &(kmem->pages[n + i - 1]);
                break;
            } else {
                i += mem_free;
                continue;
            }
        } else {
            i += mem_free;
            continue;
        }
    }

    if (frame == NULL) {
        // TODO :  ENOENT
        return NULL;
    }

    void * ptr = (void *)frame->address;

    if (n < 32 && left_over < (PAGE_SIZE / 2)) {
        end_frame->free = PAGE_SIZE - left_over;

        uint8 sector_id = 0;
        if (size <= SECTOR0_BOUND){
            sector_id = 0;
        } else if (size <= SECTOR1_BOUND) {
            sector_id = 1;
        } else if (size <= PAGE_SIZE) {
            sector_id = 2;
        }
        vm_sector * sector = &(kmem->sectors[sector_id]);
        for (uint32 i = 0; i < BKTSZ; i++) {
            if (end_frame->free > sector->actbck[i]->free) {
                if (sector->largest_free == sector->actbck[i]) {
                    sector->largest_free = end_frame;
                }
                sector->actbck[i] = end_frame;

                vaddr offset = 0;
                vm_node *node = get_alloc_node();
                vm_atom *atom = node->data;

                atom->size = left_over;
                atom->page_ptr = end_frame;
                atom->offset = offset;

                lb_llist_push_back(&(end_frame->allocs), node);

                end_frame->status = 1;

                if (sector->dirty == 32) {
                    if (sector->largest_free->free < end_frame->free) {
                        sector->largest_free = end_frame;
                    }
                }
            }
        }
    }
    return ptr;
}

void *
kalloc(usize size)
{
    spin_lock(vm_global_lock);

    void *ret = NULL;
    int16 sector_id = -1;
    if (size <= SECTOR0_BOUND){
        sector_id = 0;
    } else if (size <= SECTOR1_BOUND) {
        sector_id = 1;
    } else if (size <= PAGE_SIZE) {
        sector_id = 2;
    }

    if (sector_id >= 0) ret = vm_alloc(size, sector_id);
    else ret = vm_alloc_multpages(size);
    //void *ret = (void *) R_PADDR((paddr) vm_alloc(size, sector_id));

    /* Hack */
    int32 status = ESUCCESS;
    if (ret != NULL) {
        // check if the target page is already mapped
        // ret val does not cross page boundries
        if (get_paddr(read_cr3(), (uintptr) ret) == (uintptr) NULL) {
            uintptr frame = (uintptr) pmalloc(PAGE_SIZE);
            status = (frame != (uintptr) NULL) && (map_vmem(read_cr3(), (uintptr) ret, frame) != ESUCCESS);
        }
    }

    if (status != ESUCCESS) {
        kfree(ret);
        ret = NULL;
    } else {
        flush_tlb();
    }

    spin_unlock(vm_global_lock);
    return ret;
}

void
kfree(void *ptr)
{
    spin_lock(vm_global_lock);

    vaddr num_ptr = (vaddr) ptr;
    uint64 index = get_index(num_ptr);

    vm_object * page = &(kmem->pages[index]);

    if (page->status == 3) {
        // Free head of multi page alloc
        page->status = 0;

        page = &(kmem->pages[page->free + index - 1]);
        num_ptr = (vaddr) page->address;
    }

    usize offset = num_ptr - page->address;
    
    vm_node * tail = page->allocs.tail;

    while (tail) {
        vm_atom * atom = tail->data;
        if (atom->offset == offset && atom->page_ptr == page) {
            page->free += atom->size;
            
            /*vm_node * prev = head->prev;
            if (!prev) {
                page->allocs.head = head->next;
            } else {
                prev->next = head->next;
            }*/
            
            lb_llist_remove_by_ref(&(page->allocs), tail);

            if (page->allocs.size == 0) {
                page->allocs.head = 0x0;
                page->allocs.tail = 0x0;
            }

            spin_unlock(vm_global_lock);
            if (kmem->global_alloc->size >= 320) {
                kfree(atom);
                kfree(tail);
            }

            return;
        }
        if (tail == tail->next) break;
        tail = tail->next;
    }

    spin_unlock(vm_global_lock);
}

