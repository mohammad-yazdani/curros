/* Virtual memory management header */

// TODO : struct for global kernel memory
struct vmem {};
// TODO : struct for page table
struct vm_table {};
// TODO : struct for page
struct vm_page {};

int init_vm(struct vmem * kernmem);

struct vm_page * kalloc();
void kfree(struct vm_page * page);

