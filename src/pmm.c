#include "pmm.h"
#include "memory_layout.h"

void
mem_phase0(void)
{
	mem_state = 0;
	freelist = 0;
}

paddr
alloc_ppage(uint32 n)
{
	paddr allocated;
	
	if (mem_state == 0) {
		// TODO : This is init phase allocator. Branch off logic
		uint64 offset = freelist * PAGESZ;
		allocated = KERNEL_SPACE - offset;
		if (allocated == MEM_MAPPED_IO) return 0;
		freelist += n;
		return allocated;
	} else {
		// TODO : Here goes the code for pmm after stage 0
		return 0;
	}
}

void
pfree(paddr p)
{
	(void)p;
}

paddr
pmalloc(uint32 size)
{
	(void)size;
	uint8 left_over = size % PAGESZ;
	uint32 num_pages = (size / PAGESZ) + (left_over > 0);
	return alloc_ppage(num_pages);
}

