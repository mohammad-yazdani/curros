/* Virtual memory management header */

#include "types.h"


// TODO : struct for page
typedef struct virtual_page {
    
} vm_page;

typedef struct vm_page_table {
    uint64 num_page;
    vm_page * pages;
} vm_map;

typedef struct vmm {
    uint64 num_ent;
    vm_map * entries;
    // TODO : MORE entires 
} vmem;

void init_vm(vmem * kernmem);

void * kalloc(usize size);
void kfree(void * ptr);

