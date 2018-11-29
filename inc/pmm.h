/* Header file for physical memory management */

#include "types.h"

uint8 mem_state;
uint32 freelist;

paddr pmalloc(uint32 size); // Allocate a physical page
void pfree(paddr p); // Free physical page

void mem_phase1(uint64 base, uint64 size);
uint8 get_mem_phase();
paddr alloc_ppage(uint32 n);

