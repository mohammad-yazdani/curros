#include "pmm.h"
#include "memory_layout.h"

uint64 KHEAP_LO = 0;
uint64 KHEAP_HI = 0;

uint8
get_mem_phase()
{
	return mem_state;
}

void
mem_phase1(uint64 base, uint64 size)
{
	// Use this to get higher available RAM
	mem_state = (base > 0);
	if (mem_state) {
		KHEAP_HI = base - size;
		KHEAP_LO = base;
		freelist = 1;
	}
}

paddr
alloc_ppage(uint32 n)
{
	paddr allocated;
	
	if (mem_state == 1) {
		// TODO : This is init phase allocator. Branch off logic
		uint64 offset = freelist * (PAGESZ * n);
		allocated = KHEAP_LO + offset;
		if (allocated == MEM_MAPPED_IO) return 0;
		freelist += n;
		
		// TODO : TEMP Zero out
		uint64 * zp = (uint64 *) allocated;
		*zp = 0;
		
		return allocated;
	} else {
		// TODO : Here goes the code for pmm after stage 1
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

