/* Header file for physical memory management */

#include "types.h"
#include "llist.h"

typedef struct llist memlist;
typedef struct llist_node memnode;

typedef struct allocation_unit {
    paddr head;
    uint64 size;
    uint8 status;
    paddr end;
} allocu;

void * pmalloc(uint32 size); // Allocate a physical page
void pfree(paddr p); // Free physical page

// TODO : Setup methods
void mem_phase1(uint64 base, uint64 size);
uint8 get_mem_phase();


