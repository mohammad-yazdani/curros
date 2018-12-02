#pragma once

#include "cdef.h"
#include "memory_layout.h"

#define PML4_ENTRY(vaddr) ((vaddr >> 39) & 0x1FF)
#define PDPT_ENTRY(vaddr) ((vaddr >> 30) & 0x1FF)
#define PD_ENTRY(vaddr) ((vaddr >> 21) & 0x1FF)
#define PT_ENTRY(vaddr) ((vaddr >> 12) & 0x1FF)

#define KERNEL_PAGE_SIZE (PAGE_SIZE)

uintptr get_paddr(uint64 cr3, uintptr vaddr);

int32 map_vmem(uint64 cr3, uintptr virt_addr, uintptr phys_addr);
