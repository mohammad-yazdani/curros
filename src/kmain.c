#include "multiboot2.h"
#include "pmm.h"

void kmain(void* multiboot_info)
{
    /* What goes here is up to you */
    UNREFERENCED(multiboot_info);

	// Init mem phase 0
	mem_phase0();


	paddr test_mem = 0;
    while(!test_mem)
    { 
    	test_mem = pmalloc(4096);
    	(void)test_mem;
    }
}
