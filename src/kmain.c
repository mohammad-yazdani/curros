#include "multiboot2.h"
#include "intr.h"
#include "print.h"
#include "clib.h"
#include "cpu.h"
#include "thread.h"
#include "proc.h"
#include "pmm.h"
#include "vmm.h"
#include "elf64.h"

typedef struct multiboot_tag_module mod_tag;

void pmm_test();
void vmm_test();

void kmain(mbentry * mb)
{
	char * ld_name = 0x0;
	mod_tag * module = 0x0;

	parse_mb2(mb, (void **)(&module), &ld_name);
	void * exec_entry = elf_load_file((void *) module->mod_start);
	(void) exec_entry;

	pmm_init();
	pmm_test();

	init_vm();
	vmm_test();

	print_init();
	intr_init();

	// Here is the first page table
	while (1) {}
}


/* TESTS */
void
pmm_test()
{
    int32 *llmem1 = pmalloc(4096);
    *llmem1 = 14;
    int32 *llmem2 = pmalloc(4096);
    *llmem2 = *llmem1 + 12;
    (void) llmem2;
    pfree((paddr) llmem2);
    pfree((paddr) llmem1);
}

typedef struct dummys
{
    uint16 test0;
    int64 test1;
} ds;

void
vmm_test()
{
	uint64 * test_int = kalloc(sizeof(uint64));
	ds * dummy = kalloc(sizeof(ds));
	dummy->test0 = 34;
	dummy->test1 = 12234322;
	*test_int = 345;
	
	kfree(dummy);
	kfree(test_int);
}

