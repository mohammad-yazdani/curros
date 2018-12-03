#include "error.h"
#include "proc.h"
#include "pmm.h"
#include "vmm.h"
#include "thread.h"
#include "elf64.h"
#include "cpu.h"
#include "paging.h"
#include "memory_layout.h"

static struct spin_lock proc_list_lock;
static struct llist proc_list;
static uint32 proc_id;

int32 proc_init(void* k_routine)
{
    uint32 tid;
    int32 ret = ESUCCESS;
    spin_init(&proc_list_lock);
    lb_llist_init(&proc_list);
    proc_id = 0;

    // hack to make the first process
    struct pcb *pcb = kalloc(sizeof(struct pcb));
    if (pcb == NULL)
    {
        ret = ENOMEM;
    }

    if (ret == ESUCCESS)
    {
        pcb->cr3 = read_cr3();
        pcb->proc_id = (uint32) xinc_32((int32 *) &proc_id, 1);
        spin_init(&pcb->lock);
        lb_llist_init(&pcb->threads);

        ret = thread_create(pcb, k_routine, NULL, &tid);
    }

    if (ret != ESUCCESS)
    {
        kfree(pcb);
    }

    return ret;
}

int32 proc_create(void* elf64, uint32 *pid)
{
    int32 ret = ESUCCESS;

    // allocate struct pcb
    struct pcb *pcb = kalloc(sizeof(struct pcb));
    if (pcb == NULL)
    {
        ret = ENOMEM;
    }

    // allocate pml4
    uintptr cr3 = (uintptr) NULL;
    if (ret == ESUCCESS)
    {
        cr3 = (uintptr) pmalloc(PAGE_SIZE);
        if (cr3 == (uintptr) NULL)
        {
            ret = ENOMEM;
        }
    }

    void* entry = NULL;
    bool pushed = FALSE;
    if (ret == ESUCCESS)
    {
        // init pcb
        pcb->cr3 = cr3;
        pcb->proc_id = (uint32) xinc_32((int32 *) &proc_id, 1);
        spin_init(&pcb->lock);
        lb_llist_init(&pcb->threads);

        // write page tables
        uintptr cur_cr3 = read_cr3();
        uint64 *cur_pml4 = R_PADDR(cur_cr3);

        uint64 *pml4 = R_PADDR(cr3);
        // identity map the kernel space
        for (uint32 i = PML4_ENTRY(K_START); i < 512; i++)
        {
            pml4[i] = cur_pml4[i];
        }

        // register the process
        spin_lock(&proc_list_lock);
        lb_llist_push_front(&proc_list, &pcb->list_node);
        spin_unlock(&proc_list_lock);
        pushed = TRUE;

        ret = elf_load_file(pcb, elf64, &entry);
    }

    if (ret == ESUCCESS)
    {
        // create the thread
        uint32 tid;
        ret = thread_create(pcb, entry, (void*)0x13141516, &tid);
    }

    if (ret == ESUCCESS)
    {
        *pid = pcb->proc_id;
    }
    else
    {
        if (pcb != NULL)
        {
            if (pushed)
            {
                spin_lock(&proc_list_lock);
                lb_llist_remove_by_ref(&proc_list, &pcb->list_node);
                spin_unlock(&proc_list_lock);
            }
            kfree(pcb);
        }

        // need to properly unmap everything
        if (cr3 != (uintptr) NULL)
        {
            pfree(cr3);
        }
    }

    return ret;
}
