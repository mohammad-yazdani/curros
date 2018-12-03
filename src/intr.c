#include "intr.h"
#include "cpu.h"
#include "clib.h"
#include "memory_layout.h"
#include "print.h"
#include "paging.h"
#include "thread.h"
#include "error.h"

/**
 * IDT Defns
 */
#define GATE_DPL_0 (0ull << 13)
#define GATE_DPL_1 (1ull << 13)
#define GATE_DPL_2 (2ull << 13)
#define GATE_DPL_3 (3ull << 13)
#define GATE_PRESENT (1ull << 15)
#define GATE_TYPE_CALL (12ull << 8)
#define GATE_TYPE_INTERRUPT (14ull << 8)
#define GATE_TYPE_TRAP (15ull << 8)

/**
 GDT Defns
**/
#define SEG_GRANULARITY (1ull << 55)
#define SEG_LONG (1ull << 53)
#define SEG_DPL_0 (0ull << 45)
#define SEG_DPL_1 (1ull << 45)
#define SEG_DPL_2 (2ull << 45)
#define SEG_DPL_3 (3ull << 45)
#define SEG_PRESENT (1ull << 47)
#define SEG_CODE_DATA (1ull << 44)
#define SEG_TYPE_DATA_RW (2ull << 40)
#define SEG_TYPE_DATA_R (0ull << 40)
#define SEG_TYPE_CODE_X (8ull << 40)
#define SEG_TYPE_CODE_XR (10ull << 40)
#define SEG_TYPE_CODE_XC (12ull << 40)
#define SEG_TYPE_CODE_XRC (14ull << 40)
#define SEG_AVAILABLE (1ull << 52)
#define SEG_32_BITS (1ull << 54)

// APIC Interrupt vectors
#define EDX_MASK_APIC (1 << 9)
#define MSR_APIC_BASE (0x1b)
#define MSR_MASK_APIC (1 << 11)

// PIC defs
#define PIC1_COMMAND    (0x20)
#define PIC1_DATA    (PIC1_COMMAND+1)
#define PIC2_COMMAND    (0xa0)
#define PIC2_DATA    (PIC2_COMMAND+1)

#define APIC_REG_SPURIOUS (0xF0)
#define APIC_REG_TIMER (0x320)
#define APIC_REG_EOI (0xB0)
#define APIC_REG_TIMER_DIV (0x3E0)
#define APIC_REG_TIMER_INIT_CNT (0x380)
#define APIC_REG_TIMER_CUR_CNT (0x390)
#define APIC_REG_ID (0x20)
#define APIC_REG_LINT0 (0x350)
#define APIC_REG_LINT1 (0x360)
#define APIC_REG_ICR (0x300)

#define IPI_DLM_FIXED (0 << 8)
#define IPI_DLM_START_UP (6 << 8)

#define IPI_DSM_PHYS (0 << 11)
#define IPI_DSM_LOGI (1 << 11)
#define IPI_STATUS_MASK (1 << 12)

#define IPI_LVL_ASS (1 << 15)
#define IPI_LVL_DEA (0 << 15)

// base on 1Ghz CPU
#define APIC_TIMER_INIT_CNT (10000000)
#define REG_SPURIOUS_APIC_ENABLE (1 << 8)

extern void *intr_stub_array[NUM_IDT_DESC];

static void *apic_base;
static struct gdtr gdtptr;
static struct idtr idtptr;
static struct gdt_desc gdt[NUM_GDT_DESC];
static struct idt_desc idt[NUM_IDT_DESC];
static intr_handler intr_disp_tbl[NUM_IDT_DESC];

static void write_idt_desc(void *gate, uint64 offset, uint32 selector, uint32 attr)
{
    ((uint8 *) gate)[0] = (uint8) (offset & 0xFF);
    ((uint8 *) gate)[1] = (uint8) ((offset >> 8) & 0xFF);
    ((uint8 *) gate)[2] = (uint8) (selector & 0xFF);
    ((uint8 *) gate)[3] = (uint8) ((selector >> 8) & 0xFF);
    ((uint8 *) gate)[4] = (uint8) (attr & 0xFF);
    ((uint8 *) gate)[5] = (uint8) ((attr >> 8) & 0xFF);
    ((uint8 *) gate)[6] = (uint8) ((offset >> 16) & 0xFF);
    ((uint8 *) gate)[7] = (uint8) ((offset >> 24) & 0xFF);
    ((uint8 *) gate)[8] = (uint8) ((offset >> 32) & 0xFF);
    ((uint8 *) gate)[9] = (uint8) ((offset >> 40) & 0xFF);
    ((uint8 *) gate)[10] = (uint8) ((offset >> 48) & 0xFF);
    ((uint8 *) gate)[11] = (uint8) ((offset >> 56) & 0xFF);
    ((uint8 *) gate)[12] = 0;
    ((uint8 *) gate)[13] = 0;
    ((uint8 *) gate)[14] = 0;
    ((uint8 *) gate)[15] = 0;
}

static void write_gdt_desc(void *gdt, uint32 base, uint32 limit, uint64 attr)
{
    uint64 seg_desc = (((uint64) base & 0xFFFF) << 16) | ((((uint64) base >> 16) & 0xFF) << 32) |
                      ((((uint64) base >> 24) & 0xFF) << 56) | ((uint64) limit & 0xFFFF) |
                      ((((uint64) limit >> 16) & 0xF) << 48) | attr;
    ((uint8 *) gdt)[0] = (uint8) (seg_desc & 0xFF);
    ((uint8 *) gdt)[1] = (uint8) ((seg_desc >> 8) & 0xFF);
    ((uint8 *) gdt)[2] = (uint8) ((seg_desc >> 16) & 0xFF);
    ((uint8 *) gdt)[3] = (uint8) ((seg_desc >> 24) & 0xFF);
    ((uint8 *) gdt)[4] = (uint8) ((seg_desc >> 32) & 0xFF);
    ((uint8 *) gdt)[5] = (uint8) ((seg_desc >> 40) & 0xFF);
    ((uint8 *) gdt)[6] = (uint8) ((seg_desc >> 48) & 0xFF);
    ((uint8 *) gdt)[7] = (uint8) ((seg_desc >> 56) & 0xFF);
}

static void disable_pic()
{
    out_8(PIC1_COMMAND, 0x11);  // starts the initialization sequence (in cascade mode)
    out_8(PIC2_COMMAND, 0x11);

    out_8(PIC1_DATA, 32);  // ICW2: Master PIC vector offset
    out_8(PIC2_DATA, 40);  // ICW2: Slave PIC vector offset

    out_8(PIC1_DATA, 4);   // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    out_8(PIC2_DATA, 2);   // ICW3: tell Slave PIC its cascade identity (0000 0010)

    out_8(PIC1_DATA, 0x1); // set their modes to 8086
    out_8(PIC2_DATA, 0x1);

    out_8(PIC1_DATA, 0xff);
    out_8(PIC2_DATA, 0xff);
}

static void write_apic_reg(uint32 reg, uint32 val)
{
    *(uint32 *) ((uintptr) apic_base + reg) = val;
}

static void write_apic_reg_64(uint32 reg, uint64 val)
{
    *(uint64 *) ((uintptr) apic_base + reg) = val;
}

static uint32 read_apic_reg(uint32 reg)
{
    return *(uint32 *) ((uintptr) apic_base + reg);
}

static uint64 read_apic_reg_64(uint32 reg)
{
    return *(uint64 *) ((uintptr) apic_base + reg);
}

static int32 init_apic()
{
    uint32 eax = 1, ebx, ecx, edx;
    cpuid(&eax, &ebx, &ecx, &edx);

    if (!(edx & EDX_MASK_APIC))
    {
        return ENOSUPPORT;
    }

    disable_pic();

    // hardware enable APIC
    ecx = MSR_APIC_BASE;
    read_msr(&ecx, &edx, &eax);
    apic_base = R_PADDR(eax & 0xFFFF0000);
    eax |= MSR_MASK_APIC;
    write_msr(&ecx, &edx, &eax);
    kprintf("APIC base address: 0x%x\n", (uint64) apic_base);

    // map spurious interrupt and software enable APIC
    uint32 reg = read_apic_reg(APIC_REG_SPURIOUS);
    write_apic_reg(APIC_REG_SPURIOUS, reg | REG_SPURIOUS_APIC_ENABLE);

    // Mask LINT0 LINT1
    reg = read_apic_reg(APIC_REG_LINT0);
    write_apic_reg(APIC_REG_LINT0, reg | (1 << 16));
    reg = read_apic_reg(APIC_REG_LINT1);
    write_apic_reg(APIC_REG_LINT1, reg | (1 << 16));

    kprintf("Initializing APIC timer...\n");

    // configure APIC timer
    write_apic_reg(APIC_REG_TIMER_DIV, 0xB); // 0x1011, divide by 1
    write_apic_reg(APIC_REG_TIMER_INIT_CNT, APIC_TIMER_INIT_CNT);
    write_apic_reg(APIC_REG_TIMER_CUR_CNT, APIC_REG_TIMER_INIT_CNT);
    write_apic_reg(APIC_REG_TIMER, 0xFF020000 | INTR_VEC_TIMER); // periodic

    // unblock all interrupts
    write_cr8(0);

    return ESUCCESS;
}

static void *timer_intr_handler(struct intr_frame *frame)
{
    struct tcb *cur = get_cur_thread();

    if(cur != NULL)
    {
        // hack to get away first scheduling 1st time
        // save the stack pointer of the current thread
        cur->rsp0 = (uint64) frame;
    }

    // run the scheduler
    thread_schedule();

    cur = get_cur_thread();
    // now cur is the next thread to run
    // swap address space
    // which shouldn't page fault as we are in kernel
    write_cr3(cur->proc->cr3);
    flush_tlb();

    // now we are in the target address space
    // the only thing to do is to switch stack, which will be done in ASM handler
    return (void *) cur->rsp0;
}

int32 intr_init()
{
    int32 ret;
    kprintf("Initializing interrupts...\n");
    // init GDT
    write_gdt_desc(&gdt[GDT_NULL], 0, 0, 0); // empty desc
    write_gdt_desc(&gdt[GDT_K_CODE], 0, 0xFFFFFFFF,
                   SEG_LONG | SEG_DPL_0 | SEG_PRESENT | SEG_GRANULARITY | SEG_CODE_DATA |
                   SEG_TYPE_CODE_XR); // kernel code
    write_gdt_desc(&gdt[GDT_K_DATA], 0, 0xFFFFFFFF,
                   SEG_LONG | SEG_DPL_0 | SEG_PRESENT | SEG_GRANULARITY | SEG_CODE_DATA |
                   SEG_TYPE_DATA_RW); // kernel data
    write_gdt_desc(&gdt[GDT_U_CODE], 0, 0xFFFFFFFF,
                   SEG_LONG | SEG_DPL_3 | SEG_PRESENT | SEG_GRANULARITY | SEG_CODE_DATA |
                   SEG_TYPE_CODE_XR); // user code
    write_gdt_desc(&gdt[GDT_U_DATA], 0, 0xFFFFFFFF,
                   SEG_LONG | SEG_DPL_3 | SEG_PRESENT | SEG_GRANULARITY | SEG_CODE_DATA |
                   SEG_TYPE_DATA_RW); // user data

    gdtptr.size = GDT_SIZE - 1;
    gdtptr.offset = (uintptr) gdt;
    flush_gdt(&gdtptr, SEL(GDT_K_CODE, 0, 0), SEL(GDT_K_DATA, 0, 0));

    idtptr.offset = (uintptr) idt;
    idtptr.size = IDT_SIZE - 1;
    for (int i = 0; i < NUM_IDT_DESC; i++)
    {
        write_idt_desc(&idt[i], (uintptr) intr_stub_array[i],
                       SEL(GDT_K_CODE, 0, 0),
                       GATE_DPL_0 | GATE_TYPE_INTERRUPT | GATE_PRESENT);
    }
    mem_set(intr_disp_tbl, 0, sizeof(intr_disp_tbl));
    flush_idt(&idtptr);
    ret = init_apic();

    BOCHS_BREAK;

    if (ret == ESUCCESS)
    {
        // enable interrupt flag but mask all external interrupts
        WRITE_IRQ(0xf);
        sti();

        set_intr_handler(INTR_VEC_TIMER, timer_intr_handler);
    }

    return ret;
}

void set_intr_handler(uint32 vec, intr_handler handler)
{
    intr_disp_tbl[vec] = handler;
}

void stop_cpu()
{
    // stop interrupt and halt
    cli();
    hlt();
}

// this returns info returned by intr_disp_tbl
// in case the assembly stub needs them (e.g. ctx swap)
void *intr_dispatcher(uint32 vec, struct intr_frame *frame)
{
    void *ret = NULL;

    if (intr_disp_tbl[vec] != NULL)
    {
        ret = intr_disp_tbl[vec](frame);
    }
    else
    {
        if (vec <= INTR_LIMIT_INTEL)
        {
            kprintf("[PANIC] Exception %d has no handler. RIP: 0x%x  ERR:0x%x\n", (uint64) vec, (uint64) frame->rip,
                    (uint64) frame->error_code);
            stop_cpu();
        }
        else
        {
            kprintf("[WARN] Interrupt %d has no handler. RIP: 0x%x\n", (uint64) vec, (uint64) frame->rip);
        }
    }

    if (vec >= INTR_VEC_TIMER && vec != INTR_VEC_SPURIOUS)
    {
        // if it's delivered by local APIC, signal EOI
        write_apic_reg(APIC_REG_EOI, 0);
    }

    return ret;
}

static uint32 get_core()
{
    return read_apic_reg(APIC_REG_ID) >> 24;
}


void send_ipi(uint32 vec)
{
    // we decide to not support multicore for now. So hardcode to the current core
    // used to emulate IO stuff
    uint64 reg = ((uint64) get_core() << 56) | IPI_DLM_FIXED | IPI_DSM_PHYS | IPI_LVL_ASS | (vec & 0xff);

    // block all interrupts because there is no context
    uint64 irq = READ_IRQ();
    WRITE_IRQ(0xf);

    write_apic_reg_64(APIC_REG_ICR, reg);
    // block until successfully delivered
    while (reg & IPI_STATUS_MASK)
    {
        reg = read_apic_reg_64(APIC_REG_ICR);
    }

    WRITE_IRQ(irq);
}

