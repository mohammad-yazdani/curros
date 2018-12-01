/* Header file for physical memory management */

#include "types.h"
#include "llist.h"

typedef struct llist memlist;
typedef struct llist_node memnode;

typedef struct allocation_unit {
    paddr head;
    uint64 size;
    uint8 status;
} allocu;

void * pmalloc(uint32 size); // Allocate a physical page
void pfree(paddr p); // Free physical page

// TODO : Setup methods
void pmm_init();
uint8 get_mem_phase();
void set_mmap_low(uint64 low, uint64 size);
void set_mmap_high(uint64 high, uint64 size);


