#include "multiboot2.h"
#include "intr.h"
#include "print.h"
#include "clib.h"
#include "cpu.h"
#include "thread.h"
#include "proc.h"
#include "pmm.h"
#include "syscall.h"
#include "error.h"
#include "vmm.h"
#include "elf64.h"

void kthread1(void *arg);

void kthread2(void *arg);

void kproc(void *arg);

void *_module;

void vmm_test_sectors();

void kmain(mbentry *mb)
{
    char *ld_name = 0x0;
    mod_tag *module = 0x0;
    uint64 mem_low;
    uint64 mem_high;
    int32 status;
    print_init();

    parse_mb2(mb, (void **) &module, &ld_name, &mem_low, &mem_high);

    kprintf("Bootloader: %s\n", ld_name);
    kprintf("User executable loaded at 0x%x size %d.\n", (uint64) R_PADDR(module->mod_start), (uint64) (module->mod_end - module->mod_start));

    kprintf("Initializing interrupt...\n");
    status = intr_init();
    kprintf("Initializing system call...\n");
    syscall_init();
    KASSERT(status == ESUCCESS);

    _module = R_PADDR(module->mod_start);

    kprintf("Initializing PMM...\n");
    pmm_init(mem_low, mem_high);
    kprintf("Initializing VMM...\n");
    struct spin_lock vmlk = {0};
    init_vm(&vm);

    kprintf("Initializing threads...\n");
    thread_init();
    kprintf("Initializing processes...\n");
    status = proc_init((void *) kproc);
    KASSERT(status == ESUCCESS);

    // unmask all interrupts
    WRITE_IRQ(0x0);

    send_ipi(INTR_VEC_TIMER);
    KASSERT(FALSE);
}

#define THREAD_DELAY (5000000*4)

static uint32 kt1id;
static uint32 kt2id;

void kthread1(void *arg)
{
    UNREFERENCED(arg);
    uint64 i = 0;
    while (i != 0xFFFFFFFFFF)
    {
        poor_sleep(THREAD_DELAY);
        kprintf("[KThread1] %d\n", i);

        if (i == 10)
        {
            kprintf("[KThread1] Blocking KThread 2... \n", i);
            thread_block(kt2id);
        }

        if (i == 20)
        {
            kprintf("[KThread1] Resuming KThread 2... \n", i);
            thread_resume(kt2id);
        }

        i++;
    }
}

void kthread2(void *arg)
{
    UNREFERENCED(arg);
    bool thread1exited = FALSE;
    uint64 i = 0;
    int32 ex;
    while (i != 0xFFFFFFFFFF)
    {
        poor_sleep(THREAD_DELAY);

        kprintf("[KThread2] %d\n", i);

        if (i == 40)
        {
            kprintf("[KThread2] Stopping KThread1 with exit code 0xdeadbeef\n", i);
            thread_stop(kt1id, 0xDEADBEEF);
        }

        if (!thread1exited && thread_get_exit_code(kt1id, &ex) == ESUCCESS)
        {
            kprintf("[KThread2] KThread1 exited with 0x%x\n", (uint64) ex);
            thread1exited = TRUE;
        }
        i++;
    }
}

void kproc(void *arg)
{
    // the idle thread
    UNREFERENCED(arg);
    int32 status;
    status = thread_create(get_cur_thread()->proc, (void *) kthread1, NULL, &kt1id);
    kprintf("[KIdle] Created KThread1... 0x%x\n", (uint64) status);
    status = thread_create(get_cur_thread()->proc, (void *) kthread2, NULL, &kt2id);
    kprintf("[KIdle] Created KThread2... 0x%x\n", (uint64) status);
    uint32 id;
    status = proc_create(_module, &id);
    kprintf("[KIdle] Created process from module... 0x%x\n", (uint64) status);
    while (1)
    {
#ifdef KDBG
        kprintf("[KIdle] Yielding...\n");
#endif
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

