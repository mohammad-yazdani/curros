#include "cdef.h"
#include "multiboot2.h"
#include "intr.h"
#include "print.h"
#include "clib.h"
#include "cpu.h"

void kmain(void* multiboot_info)
{
    UNREFERENCED(multiboot_info);

    print_init();
    clear_screen();
    intr_init();
    while(1)
    {

    }
}
