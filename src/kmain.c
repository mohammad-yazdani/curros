#include "multiboot2.h"
#include "types.h"
#include "pmm.h"

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
			mem_phase1(entry.addr, entry.len);
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

void kmain(mbentry * mb)
{
	uint32 next_entry;
	next_entry = ((uint32) mb) + 8;
	mtag * tag = (mtag *) next_entry;

	while (!get_mem_phase()) {
		tag += proccess_tag(tag);
	}

	paddr ppage = pmalloc(4096);
	uint32 * pintpage = (uint32 *)ppage;
	*pintpage = 12;
	while (1) {}
	/*
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
