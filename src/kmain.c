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

void ktest1(void *arg);

void ktest2(void *arg);

void kproc(void *arg);

void pmm_test();

void vmm_test();

void kmain(mbentry *mb)
{
    int32 status;

    print_init();

    status = intr_init();
    KASSERT(status == ESUCCESS);

    char *ld_name = 0x0;
    mod_tag *module = 0x0;
    uint64 mem_low;
    uint64 mem_high;
    parse_mb2(mb, (void **) &module, &ld_name, &mem_low, &mem_high);

    kprintf("%s\n", ld_name);
    kprintf("Module loaded at 0x%x size %d.\n", (uint64) module->mod_start,
            (uint64) (module->mod_end - module->mod_start));

    pmm_init(mem_low, mem_high);

    init_vm();

    thread_init();
    status = proc_init((void*)kproc);
    KASSERT(status == ESUCCESS);

    // unmask all interrupts
    WRITE_IRQ(0x0);

    send_ipi(INTR_VEC_TIMER);
    KASSERT(FALSE);
}


static uint32 t1id;
static uint32 t2id;

void ktest1(void *arg)
{
    UNREFERENCED(arg);
    uint64 i = 0;
    while (i != 0xFFFFFFFFFF)
    {
        poor_sleep(5000000);
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
        poor_sleep(5000000);
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
    // the idle thread
    UNREFERENCED(arg);
    int32 status;
    status = thread_create(get_cur_thread()->proc, (void *) ktest1, NULL, &t1id);
    kprintf("Create thread1 %d\n", (uint64) status);
    status = thread_create(get_cur_thread()->proc, (void *) ktest2, NULL, &t2id);
    kprintf("Create thread2 %d\n", (uint64) status);
    while (1)
    {
        kprintf("Idle thread\n");
        thread_yield();
    }
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

