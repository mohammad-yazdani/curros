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

typedef struct multiboot_entry
{
    uint32 total_size;
    uint32 reserved;
} mbentry;
typedef struct multiboot_tag mtag;

void
proccess_mmap(struct multiboot_tag_mmap *mmap)
{
    struct multiboot_mmap_entry entry;
    uint16 num_entries = (mmap->size - sizeof(*mmap)) / mmap->entry_size;
    for (uint16 i = 0; i < num_entries; i++)
    {
        entry = mmap->entries[i];
        kprintf("ADDR: 0x%x   LEN: 0x%x  TYPE: %x\n", (uint64)entry.addr, (uint64)entry.len, (uint64)entry.type);
        if (entry.type == 1)
        {
            if (entry.addr)
            {
                kprintf("Set high: 0x%x\n", entry.addr);
                set_mmap_high(entry.addr, entry.len);
            }
            else
            {
                kprintf("Set low: 0x%x\n", entry.addr);
                set_mmap_low(entry.addr, entry.len);
            }
        }
    }
}

uint32
proccess_tag(mtag *tag)
{
    struct multiboot_tag_mmap *mmap_tag = 0;
    struct multiboot_tag_load_base_addr *base_tag = 0;

    switch (tag->type)
    {
        case 6:
            mmap_tag = (struct multiboot_tag_mmap *) tag;
            proccess_mmap(mmap_tag);
            break;
        case 21:
            base_tag = (struct multiboot_tag_load_base_addr *) tag;
            (void) base_tag;
            break;
        default:
            break;
    }
    return tag->size;
}

void
parse_multiboot(mbentry *mb)
{
    uint32 next_entry;
    next_entry = ((uint32) mb) + 8;
    mtag *tag = (mtag *) next_entry;

    while (1)
    {
        uint32 size = proccess_tag(tag);
        if (!size)
        { break; }
        tag += size;
    }
}

/* TESTS */
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

    kfree(dummy);
    kfree(test_int);
}

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

void kmain(void* mb)
{
    int32 status;

    print_init();

    status = intr_init();
    KASSERT(status == ESUCCESS);

    parse_multiboot(mb);

    pmm_init();
    pmm_test();

    init_vm();
    //vmm_test();


    for(int i = 0; i < 10; i++)
    {
        void* a = kalloc(48);

        kprintf("0x%x\n", (uint64)a);
    }

    while(1)
    {
        //
    }
//    proc_init();
//    thread_init();

    for(int i = 0; i < 10; i++)
    {
        void* a = kalloc(48);
        kprintf("0x%x\n", a);
    }

    uint32 id;
    status = proc_create(kproc, &id);
    KASSERT(status == ESUCCESS);

    // unmask all interrupts
    WRITE_IRQ(0x0);

    // wait for timer interrupt to fire and schedule the first thread
    while(1)
    {
    }
}
