/* Virtual memory management header */

#include "types.h"
#include "memory_layout.h"
#include "llist.h"

typedef struct llist vmll;
typedef struct llist_node vm_node;

typedef struct virtual_page {
    vaddr address; // Init to 0x0
    vaddr table_ptr;
    vmll allocs; // Linked list of vm_atom
    uint64 free;
    uint8 status;
} vm_object;

typedef struct allocation {
    usize offset;
    usize size;
    vm_object * page_ptr;
} vm_atom;

typedef struct page_table {
    uint64 id;
    usize size;
    usize free;
    vm_atom * map;
} vm_unit;

typedef struct allocation_sector {
    uint64 start;
    uint64 end;
    vm_object ** actbck;
    vm_object ** resbck;
    vm_object * largest_free;
    uint16 dirty;
} vm_sector;

typedef struct page_table_entries {
    vm_unit * groups;
    vm_sector sectors[3];
    vm_object * pages;

    vmll * global_alloc;
} vmem;

void init_vm();

void vm_issue_unit(uint64 id, usize size);

void * kalloc(usize size);
void kfree(void * ptr);

