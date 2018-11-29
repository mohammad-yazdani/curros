#include "cdef.h"
#include "multiboot2.h"
#include "intr.h"
#include "print.h"

void kmain(void* multiboot_info)
{
    /* What goes here is up to you */
    UNREFERENCED(multiboot_info);
    print_init();
    clear_screen();
    kprintf("Initializing interrupt...\n");
    intr_init();
    while(1)
    {
        
    }
}
