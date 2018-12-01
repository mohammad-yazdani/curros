#include "cdef.h"
#include "multiboot2.h"
#include "intr.h"
#include "print.h"
#include "clib.h"
#include "cpu.h"
#include "thread.h"
#include "proc.h"

void kmain(void* multiboot_info)
{
    UNREFERENCED(multiboot_info);

    print_init();
    intr_init();

    thread_init();
    while(1)
    {

    }
}
