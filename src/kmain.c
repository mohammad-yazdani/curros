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

void vmm_test_sectors();

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
    pmm_test();
    

    struct spin_lock vmlk = {0};
    init_vm(&vmlk);
    vmm_test();
    vmm_test_sectors();

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


typedef struct dummys
{
    uint16 test0;
    int64 test1;
} ds;

void
pmm_test()
{
    uint64 *test_int = R_PADDR((paddr)pmalloc(PAGE_SIZE));
    *test_int = 345;
    pfree(test_int);
    kprintf("PMM TEST OK\n");
}


void
vmm_test()
{
    uint64 *test_int = kalloc(sizeof(uint64));
    kprintf("%d\n", *test_int);
    *test_int = 2;

    uint64 test = *test_int;
    for (int i = 0; i < 10; i++)
    {
        test_int += (i * 4);
        void *a = kalloc(test);
        kprintf("VM: 0x%x\n", a);
    }

    void * page_alloc = kalloc(511);
    kfree(page_alloc);
    page_alloc = kalloc(63);
    kfree(page_alloc);
    page_alloc = kalloc(513);
    kfree(page_alloc);
    page_alloc = kalloc(PAGE_SIZE);
    kfree(page_alloc);
    page_alloc = kalloc(PAGE_SIZE + 1);

    kfree(page_alloc);
    kfree(test_int);

    kprintf("VMM TEST OK\n");
}

void
vmm_test_sectors()
{
    /*usize sec0lo = 63, sec0hi = 65, sec0 = 64;
    usize sec1lo = 511, sec1hi = 513, sec1 = 512;
    usize sec2lo = 4088, sec2hi = 240323, sec2 = PAGE_SIZE;
    */
    // TODO : Test sequentially
    // TODO : Test In loops

    // TODO : Out of order free
}

