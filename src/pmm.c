#include "pmm.h"
#include "memory_layout.h"


uint8 mem_state = 0;
memlist * freelist = 0x0;
uint64 pmem_head = 0;

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
pmm_init() 
{
	if (mem_state) {
		KHEAP_HI = MEM_HI;
		KHEAP_LO = MEM_HI + HISZ;

		// TODO : Freelist allocation hacks

		freelist = pmalloc(4096);
		lb_llist_init(freelist);

		memnode * freelist_node = (memnode *) (freelist + (sizeof(*freelist)));
		allocu * freelist_unit = (allocu *) (freelist_node + (sizeof(*freelist_node)));
		freelist_unit->size = 1;
		freelist_unit->status = 1;
		// TODO : First allocation unit points to PMM data structure.
		freelist_unit->head = (paddr) freelist;

		freelist_node->data = freelist_unit;
		
		lb_llist_push_back(freelist, freelist_node);

		// TODO : IF all is good, now we must have a llist of memory
		mem_state = 2;
	}
}

paddr
freelist_alloc(uint32 n)
{
	memnode * curr_node = freelist->head;
	while(1) {
		allocu * toalloc = curr_node->data;
		if (toalloc->size >= n && toalloc->status == 0) {
			// TODO : Free interval found
			// TODO : NOT ATOMIC
			toalloc->status = 1;
			return toalloc->head;
		}

		if (!curr_node->next) break;
		curr_node = curr_node->next;
	}

	// TODO : Alloc from front
	allocu * last_alloc = (allocu *) curr_node->data;
	paddr new_head = (paddr) last_alloc + sizeof(*last_alloc);
	memnode * new_node = (memnode *) new_head;
	allocu * new_alloc = (allocu *) (new_head + sizeof(*new_node));

	new_alloc->size = n;
	new_alloc->status = 1;
	new_alloc->head = last_alloc->head - (n * PAGESZ);

	new_node->data = new_alloc;

	lb_llist_push_back(freelist, new_node);

	return new_alloc->head;
}

paddr
alloc_ppage(uint32 n)
{
	paddr allocated;
	
	if (mem_state == 1) {
		// TODO : This is init phase allocator. Branch off logic
		uint64 offset = (pmem_head + 1) * (PAGESZ * n);
		allocated = KHEAP_LO - offset;
		if (allocated == KHEAP_HI) return 0;
		pmem_head += n;
		
		// TODO : TEMP Zero out
		uint64 * zp = (uint64 *) allocated;
		*zp = 0;
	} else if (mem_state == 2) {
		allocated = freelist_alloc(n);
	} else {
		// TODO : Here goes the code for pmm after stage 1
		allocated = 0;
	}

	return allocated;
}

void
pfree(paddr p)
{
	memnode * curr_node = freelist->head;
	while(curr_node) {
		allocu * toalloc = curr_node->data;
		if (toalloc->head == p && toalloc->status == 1 && toalloc->size > 0) {
			toalloc->status = 0;
			return;
		}

		curr_node = curr_node->next;
	}
}

void *
pmalloc(uint32 size)
{
	(void)size;
	uint8 left_over = size % PAGESZ;
	uint32 num_pages = (size / PAGESZ) + (left_over > 0);
	paddr new_alloc_paddr = alloc_ppage(num_pages);
	return (void *) new_alloc_paddr;
}
