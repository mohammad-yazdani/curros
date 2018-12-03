#include <memory_layout.h>
#include <print.h>
#include "paging.h"
#include "pmm.h"
#include "cpu.h"
#include "clib.h"
#include "error.h"

#define ENTRY_SIZE (8)

#define NEXT_LEVEL_MASK bit_field_mask(12,51)

#define BIT_PRESENT (1ull)
#define BIT_WRITEABLE (1ull << 1)
#define BIT_USER (1ull << 2)
#define BIT_WRITE_THROUGH (1ull << 3)
#define BIT_CACHE_DISABLED (1ull << 4)
#define BIT_ACCESSED (1ull << 5)
#define BIT_PS (1ull << 7)
#define BIT_EXECUTION_DISABLED (1ull << 63)

static void
write_page_table(void *const base, uintptr const p_addr, uint64 const attr)
{
    uint64 entry = (p_addr & 0xFFFFFFFFFF000) | attr;
    ((uint8 *) base)[0] = (uint8) (entry & 0xFF);
    ((uint8 *) base)[1] = (uint8) ((entry >> 8) & 0xFF);
    ((uint8 *) base)[2] = (uint8) ((entry >> 16) & 0xFF);
    ((uint8 *) base)[3] = (uint8) ((entry >> 24) & 0xFF);
    ((uint8 *) base)[4] = (uint8) ((entry >> 32) & 0xFF);
    ((uint8 *) base)[5] = (uint8) ((entry >> 40) & 0xFF);
    ((uint8 *) base)[6] = (uint8) ((entry >> 48) & 0xFF);
    ((uint8 *) base)[7] = (uint8) ((entry >> 56) & 0xFF);
}

static int32 ensure_present(uint64 *entry, uint64 attr, uintptr *alloc)
{
    uint32 ret = ESUCCESS;

    uintptr p_alloc = (uintptr) NULL;

    if (!(*entry & BIT_PRESENT))
    {
        p_alloc = (uintptr) pmalloc(KERNEL_PAGE_SIZE);
        if (p_alloc == (uintptr) NULL)
        {
            ret = ENOMEM;
        }
        else
        {
            mem_set(R_PADDR(p_alloc), 0, KERNEL_PAGE_SIZE);
            write_page_table(entry, p_alloc, attr);
        }
    }

    if (ret == ESUCCESS)
    {
        *alloc = p_alloc;
    }

    return ret;
}

int32 map_vmem(uint64 cr3, uintptr vaddr, uintptr paddr)
{
    int32 ret;

    vaddr &= ~(uintptr)0xfff;
    paddr &= ~(uintptr)0xfff;

    uint64 *pml4_e = R_PADDR(cr3 + ENTRY_SIZE * PML4_ENTRY(vaddr));
    uintptr pdpt_alloc = (uintptr) NULL;
    ret = ensure_present(pml4_e, BIT_PRESENT | BIT_WRITEABLE | (IS_KERN_SPACE(vaddr) ? 0 : BIT_USER), &pdpt_alloc);

    uint64 *pdpt_e = NULL;
    uintptr pd_alloc = (uintptr) NULL;
    if (ret == ESUCCESS)
    {
        pdpt_e = R_PADDR((*pml4_e & NEXT_LEVEL_MASK) + ENTRY_SIZE * PDPT_ENTRY(vaddr));
        ret = ensure_present(pdpt_e, BIT_PRESENT | BIT_WRITEABLE | (IS_KERN_SPACE(vaddr) ? 0 : BIT_USER), &pd_alloc);
    }

    uint64 *pd_e = NULL;
    uintptr pt_alloc = (uintptr) NULL;
    if (ret == ESUCCESS)
    {
        pd_e = R_PADDR((*pdpt_e & NEXT_LEVEL_MASK) + ENTRY_SIZE * PD_ENTRY(vaddr));
        ret = ensure_present(pd_e, BIT_PRESENT | BIT_WRITEABLE | (IS_KERN_SPACE(vaddr) ? 0 : BIT_USER), &pt_alloc);
    }

    if (ret == ESUCCESS)
    {
        uint64 *pt_e = R_PADDR((*pd_e & NEXT_LEVEL_MASK) + ENTRY_SIZE * PT_ENTRY(vaddr));
        write_page_table(pt_e, paddr, BIT_PRESENT | BIT_WRITEABLE | (IS_KERN_SPACE(vaddr) ? 0 : BIT_USER));
        kprintf("Mapped 0x%x to 0x%x\n", vaddr, paddr);
    }

    if (ret != ESUCCESS)
    {
        if (pdpt_alloc != (uintptr) NULL)
        {
            pfree(pdpt_alloc);
        }

        if (pd_alloc != (uintptr) NULL)
        {
            pfree(pd_alloc);
        }

        if (pt_alloc != (uintptr) NULL)
        {
            pfree(pt_alloc);
        }
    }

    return ret;
}

uintptr get_paddr(uint64 cr3, uintptr vaddr)
{
    uint64 pml4_e = *(uint64 *) R_PADDR(cr3 + ENTRY_SIZE * PML4_ENTRY(vaddr));

    if (!(pml4_e & BIT_PRESENT))
    {
        // not present in PML4
        return (uintptr) NULL;
    }
    uint64 pdpt_e = *(uint64 *) R_PADDR((pml4_e & NEXT_LEVEL_MASK) + ENTRY_SIZE * PDPT_ENTRY((uintptr)vaddr));

    if (!(pdpt_e & BIT_PRESENT))
    {
        return (uintptr) NULL;
    }

    if (pdpt_e & BIT_PS)
    {
        // 1GB page
        return (pdpt_e & bit_field_mask(30, 51)) + ((uintptr)vaddr & (0x3FFFFFFF));
    }

    uint64 pd_e = *(uint64 *) R_PADDR((pdpt_e & NEXT_LEVEL_MASK) + ENTRY_SIZE * PD_ENTRY((uintptr)vaddr));

    if (!(pdpt_e & BIT_PRESENT) || (pdpt_e & BIT_PS))
    {
        // don't support 2MB pages yet
        return (uintptr) NULL;
    }

    uint64 pt_e = *(uint64 *) R_PADDR((pd_e & NEXT_LEVEL_MASK) + ENTRY_SIZE * PT_ENTRY(vaddr));

    if (!(pt_e & BIT_PRESENT))
    {
        return (uintptr) NULL;
    }

    return (pt_e & bit_field_mask(12, 51)) + (uintptr)vaddr & 0xFFF;
}
