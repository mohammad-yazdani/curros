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

void ktest1(void *arg);

void ktest2(void *arg);

void kproc(void *arg);

void exec_test(void *entry, int32 *status);

void pmm_test();

void vmm_test();

void kmain(mbentry *mb)
{
    int32 status;

    print_init();
    intr_init();

    status = intr_init();
    KASSERT(status == ESUCCESS);

    char *ld_name = 0x0;
    mod_tag *module = 0x0;
    parse_mb2(mb, (void **) (&module), &ld_name);

    kprintf("%s\n", ld_name);
    kprintf("Module loaded starting at %X and ending at %X.\n", module->mod_start, module->mod_end);

    void *exec_entry = elf_load_file((void *) module->mod_start);
    kprintf("Executable load from module file. Entry at: %X\n", exec_entry);

    struct spin_lock pmlk = {0};
    struct spin_lock vmlk = {0};

    pmm_init(&pmlk);
//	pmm_test();

    init_vm(&vmlk);
//	vmm_test();

    proc_init();
    thread_init();

    // start proc 0, which is kernel proc, also null proc
    uint32 id;
    status = proc_create(kproc, &id);
    KASSERT(status == ESUCCESS);

    // unmask all interrupts
    WRITE_IRQ(0x0);

    while (1)
    {

    }

    // wait for timer interrupt to fire and schedule the first thread
    while (1)
    {}
}


static uint32 t1id;
static uint32 t2id;

void ktest1(void *arg)
{
    UNREFERENCED(arg);
    uint64 i = 0;
    while (i != 0xFFFFFFFFFF)
    {
        poor_sleep(1000000);
        kprintf("thread1...%d\n", i);

        if (i == 10)
        {
            thread_block(t2id);
        }

        if (i == 20)
        {
            thread_resume(t2id);
        }

        if (i == 30)
        {
            thread_stop(t2id, -2);
        }

        i++;
    }
}

void ktest2(void *arg)
{
    UNREFERENCED(arg);
    uint64 i = 0;
    int32 ex;
    while (i != 0xFFFFFFFFFF)
    {
        poor_sleep(1000000);
        kprintf("thread2...%d\n", i);

        if (thread_get_exit_code(t1id, &ex) == ESUCCESS)
        {
            kprintf("thread1 exited with %d\n", (uint64) ex);
        }

        i++;
    }
}

void kproc(void *arg)
{
    UNREFERENCED(arg);
    int32 status;
    status = thread_create(get_cur_thread()->proc, ktest1, NULL, &t1id);
    kprintf("Create thread1 %d\n", (uint64) status);
    status = thread_create(get_cur_thread()->proc, ktest2, NULL, &t2id);
    kprintf("Create thread2 %d\n", (uint64) status);
    while (1)
    {
        //kprintf("Idle thread\n");
        thread_yield();
    }
}

void
exec_test(void *entry, int32 *status)
{
    uint32 id;
    *status = proc_create((void (*)(void *)) entry, &id);
    KASSERT((*status) == ESUCCESS);
    kprintf("Program execution successful.\n");

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
    uint64 *test_int = kalloc(sizeof(uint64));
    ds *dummy = kalloc(sizeof(ds));
    dummy->test0 = 34;
    dummy->test1 = 12234322;
    *test_int = 345;

    for (int i = 0; i < 10; i++)
    {
        void *a = kalloc(48);
        kprintf("0x%x\n", a);
    }

    kfree(dummy);
    kfree(test_int);

    kprintf("VMM TEST OK\n");
}

