#include "multiboot2.h"
#include "pmm.h"
#include "types.h"


void kmain(void* multiboot_info)
{
    /* What goes here is up to you */
    UNREFERENCED(multiboot_info);

	// Init mem phase 0
	mem_phase0();


	paddr pmem_test = 0;
	uint32 * pptr_test = 0;
    while(!pmem_test)
    { 
    	pmem_test = pmalloc(4096);
    	pptr_test = (uint32 *) pmem_test;

    	// TODO : WRITE TEST
    	*pptr_test = 12;
    	*pptr_test = 10;
    	uint32 intest = *pptr_test;
    	(void)intest;
    } 
}
