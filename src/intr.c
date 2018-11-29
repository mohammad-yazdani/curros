#include "intr.h"
#include "cpu.h"
#include "clib.h"
#include "memory_layout.h"
#include "print.h"
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
#define MSR_MASK_APIC (11)

// PIC defs
#define PIC1_COMMAND    (0x20)
#define PIC1_DATA    (PIC1_COMMAND+1)
#define PIC2_COMMAND    (0xa0)
#define PIC2_DATA    (PIC2_COMMAND+1)


extern uint32 intr_stub_size;

extern void intr_stub_start();

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

    out_8(PIC1_DATA, 32);                 // ICW2: Master PIC vector offset
    out_8(PIC2_DATA, 40);                 // ICW2: Slave PIC vector offset

    out_8(PIC1_DATA, 4);                  // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    out_8(PIC2_DATA, 2);                  // ICW3: tell Slave PIC its cascade identity (0000 0010)

    out_8(PIC1_DATA, 0x1);          // set their modes to 8086
    out_8(PIC2_DATA, 0x1);

    out_8(PIC1_DATA, 0xffff);
    out_8(PIC2_DATA, 0xffff);

}

static int init_apic()
{
    uint32 eax = 1, ebx, ecx, edx;
    cpuid(&eax, &ebx, &ecx, &edx);
    if (!(edx & EDX_MASK_APIC))
    {
        return ENOSUPPORT;
    }

    disable_pic();

    // configure APIC


    // enable APIC
    ecx = MSR_APIC_BASE;
    read_msr(&ecx, &edx, &eax);
    eax |= MSR_MASK_APIC;
    write_msr(&ecx, &edx, &eax);

    return 0;
}

void intr_init()
{
    kprintf("Initializing GDT...\n");
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
                   SEG_TYPE_DATA_RW); // user code
    write_gdt_desc(&gdt[GDT_U_DATA], 0, 0xFFFFFFFF,
                   SEG_LONG | SEG_DPL_3 | SEG_PRESENT | SEG_GRANULARITY | SEG_CODE_DATA |
                   SEG_TYPE_DATA_RW); // user data

    gdtptr.size = GDT_SIZE - 1;
    gdtptr.offset = (uint64) gdt - KERNEL_IMAGE_VADDR;
    flush_gdt(&gdtptr, SEL(GDT_K_CODE, 0, 0), SEL(GDT_K_DATA, 0, 0));

    // init IDT
    kprintf("Initializing IDT...\n");
    idtptr.offset = (uint64) idt - KERNEL_IMAGE_VADDR;
    idtptr.size = IDT_SIZE - 1;
    for (int i = 0; i < NUM_IDT_DESC; i++)
    {
        write_idt_desc(&idt[i], (uintptr) intr_stub_start + i * intr_stub_size - KERNEL_IMAGE_VADDR,
                       SEL(GDT_K_CODE, 0, 0),
                       GATE_DPL_0 | GATE_TYPE_INTERRUPT | GATE_PRESENT);
    }
    mem_set(intr_disp_tbl, 0, sizeof(intr_disp_tbl));
    flush_idt(&idtptr);

    // init IDT
    kprintf("Initializing APIC...\n");
    init_apic();
}

void set_intr_handler(uint32 vec, intr_handler handler)
{
    intr_disp_tbl[vec] = handler;
}

void *intr_dispatcher(uint32 vec, void *frame)
{
    if (intr_disp_tbl[vec] != NULL)
    {
        frame = intr_disp_tbl[vec](vec, frame);
    }
    else
    {
        kprintf("[PANIC] Interrupt %d has no handler. Frame: 0x%x\n", (uint64) vec, (uint64) frame);
        while (1)
        {

        }
    }
    return frame;
}
