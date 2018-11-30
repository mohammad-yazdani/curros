#pragma once
#include "cdef.h"
#include "cpu.h"

#define READ_IRQ()  read_cr8()
#define WRITE_IRQ(x)  write_cr8(x)

#define NUM_IDT_DESC (256)
#define NUM_GDT_DESC (5)
#define IDT_DESC_SIZE (16)
#define GDT_DESC_SIZE (8)
#define GDT_SIZE ((NUM_GDT_DESC) * (GDT_DESC_SIZE))
#define IDT_SIZE ((NUM_IDT_DESC) * (IDT_DESC_SIZE))

#define GDT_K_CODE (1)
#define GDT_NULL (0)
#define GDT_K_DATA (2)
#define GDT_U_CODE (3)
#define GDT_U_DATA (4)
#define SEL(idx, TI, RPL) ((((uint16)idx) << 3) | (((uint16) TI) << 2) | ((uint16) RPL))

struct PACKED gdtr
{
    uint16 size;
    uint64 offset;
};

struct PACKED idtr
{
    uint16 size;
    uint64 offset;
};

struct PACKED gdt_desc
{
    uint16 seg_l;
    uint16 base_l;
    uint8 base_m;
    uint8 attr1;
    uint8 attr2;
    uint8 base_h;
};

struct PACKED idt_desc
{
    uint16 offset_l;
    uint16 seg_sel;
    uint16 attr;
    uint16 offset_m;
    uint32 offset_h;
    uint32 reserved;
};


// returns the new exception frame pointer
// interrupt handlers should EOI
typedef void* (*intr_handler)(uint32 vec, void* frame);

int32 intr_init();
void* ASM_F intr_dispatcher(uint32 vec, void *frame);
void set_intr_handler(uint32 vec, intr_handler handler);

