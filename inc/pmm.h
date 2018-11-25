/* Header file for physical memory management */

#include "types.h"

uint8 mem_state;
uint32 freelist;

paddr pmalloc(uint32 size); // Allocate a physical page
void pfree(paddr p); // Free physical page

void mem_phase0(void);
paddr alloc_ppage(uint32 n);

