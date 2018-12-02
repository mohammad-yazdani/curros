#pragma once

#include "cdef.h"

#define KERNEL_PAGE_SIZE (4096)

uintptr get_paddr(uintptr vaddr);

int32 map_vmem(uintptr virt_addr, uintptr phys_addr);
