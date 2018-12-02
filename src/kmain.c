#include "multiboot2.h"
#include "intr.h"
#include "print.h"
#include "clib.h"
#include "cpu.h"
#include "thread.h"
#include "proc.h"
#include "pmm.h"
#include "error.h"
#include "vmm.h"
#include "elf64.h"

typedef struct multiboot_tag_module mod_tag;

void ktest1(void* arg);
void ktest2(void* arg);
void kproc(void* arg);
void proc_test(int32 * status);
void pmm_test();
void vmm_test();

void kmain(mbentry * mb)
{
    int32 status;

    print_init();
    intr_init();

    status = intr_init();
    KASSERT(status == ESUCCESS);

	char * ld_name = 0x0;
	mod_tag * module = 0x0;
	parse_mb2(mb, (void **)(&module), &ld_name);

    kprintf("%s\n", ld_name);
    kprintf("Module loaded starting at %X and ending at %X.\n", module->mod_start, module->mod_end);

	void * exec_entry = elf_load_file((void *) module->mod_start);
	kprintf("Executable load from module file. Entry at: %X\n", exec_entry);

	pmm_init();
	pmm_test();

	init_vm();
	//vmm_test();

    proc_test(&status);

	// TODO : Here is the first page table

    // wait for timer interrupt to fire and schedule the first thread
	while (1) {}
}


/* TESTS */

void ktest1(void* arg)
{
    UNREFERENCED(arg);
    while(1)
    {
        poor_sleep(1000);
        kprintf("test1...\n");
    }
}

void ktest2(void* arg)
{
    UNREFERENCED(arg);
    while(1)
    {
        poor_sleep(1000);
        kprintf("test2...\n");
    }
}

void kproc(void* arg)
{
    UNREFERENCED(arg);

    uint32 id;
    cli();
    thread_create(get_cur_thread()->proc, ktest1, NULL, &id);
    kprintf("Create thread1 %d\n", id);
    list_threads();
    thread_create(get_cur_thread()->proc, ktest2, NULL, &id);
    kprintf("Create thread2 %d\n", id);
    list_threads();
    while(1)
    {
        poor_sleep(1000);
        kprintf("Idle thread...\n");
    }
}

void
proc_test(int32 * status)
{
    uint32 id;
    *status = proc_create(kproc, &id);
    KASSERT((*status) == ESUCCESS);

    // unmask all interrupts
    WRITE_IRQ(0x0);
}

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

    for(int i = 0; i < 10; i++)
    {
        void* a = kalloc(48);
        kprintf("0x%x\n", a);
        kprintf("0x%x\n", (uint64)a);
    }

	kfree(dummy);
	kfree(test_int);
}

