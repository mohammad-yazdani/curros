#include "multiboot2.h"
#include "types.h"
#include "pmm.h"
#include "vmm.h"

typedef struct multiboot_entry
{
	uint32 total_size;
	uint32 reserved;
} mbentry;
typedef struct multiboot_tag mtag;

void
proccess_mmap(struct multiboot_tag_mmap * mmap)
{
	struct multiboot_mmap_entry entry;
	uint16 num_entries = (mmap->size - sizeof(*mmap)) / mmap->entry_size;
	for (uint16 i = 0; i < num_entries; i++) {
		entry = mmap->entries[i];
		if (entry.type == 1) {
			if (entry.addr) set_mmap_high(entry.addr, entry.len);
			else set_mmap_low(entry.addr, entry.len);
		}
	}
}

uint32
proccess_tag(mtag * tag)
{
	struct multiboot_tag_mmap * mmap_tag = 0;
	struct multiboot_tag_load_base_addr * base_tag = 0;

	switch (tag->type) {
		case 6:
			mmap_tag = (struct multiboot_tag_mmap *) tag;
			proccess_mmap(mmap_tag);
			break;
		case 21:
			base_tag = (struct multiboot_tag_load_base_addr *) tag;
			(void) base_tag;
			break;
		default:
			break;
	}
	return tag->size;
}

void
parse_multiboot(mbentry * mb)
{
	uint32 next_entry;
	next_entry = ((uint32) mb) + 8;
	mtag * tag = (mtag *) next_entry;

	while (1) {
		uint32 size = proccess_tag(tag);
		if (!size) break;
		tag += size;
	}
}

/* TESTS */
void
pmm_test()
{
	int32 * llmem1 = pmalloc(4096);
	*llmem1 = 14;
	int32 * llmem2 = pmalloc(4096);
	*llmem2 = *llmem1 + 12;
	(void)llmem2;
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
	
	kfree(dummy);
	kfree(test_int);
}

void kmain(mbentry * mb)
{
	parse_multiboot(mb);

	pmm_init();
	pmm_test();

	init_vm();
	vmm_test();
	
	// Here is the first page table


	while (1) {}
	/*1
	paddr pmem_test = 0;
	//uint32 * pptr_test = 0;
    while(!pmem_test)
    { 
    	pmem_test = 1;//pmalloc(4096);
		//pptr_test = (uint32 *) pmem_test;

    	// TODO : WRITE TEST
    	*pptr_test = 12;
    	*pptr_test = 10;
    	uint32 intest = *pptr_test;
    	(void)intest;
    } 
	*/
}
