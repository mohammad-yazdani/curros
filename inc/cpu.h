#pragma once

#include "cdef.h"

void ASM_F out_8(uint16 port, uint8 data);
void ASM_F out_16(uint16 port, uint16 data);
void ASM_F out_32(uint16 port, uint32 data);

int32 ASM_F cmpxchg_32(int32 *dst, int32 old_val, int32 new_val);
int32 ASM_F xinc_32(int32* dst, int32 val);

uint8 ASM_F in_8(uint16 port);
uint16 ASM_F in_16(uint16 port);
uint32 ASM_F in_32(uint16 port);

void ASM_F flush_gdt(void *gdt_ptr, uint16 code_slct, uint16 data_slct);

void ASM_F flush_idt(void *idt_ptr);

void ASM_F cpuid(uint32 *eax, uint32 *ebx, uint32 *ecx, uint32 *edx);

void ASM_F read_msr(uint32 *ecx, uint32 *edx, uint32 *eax);

void ASM_F write_msr(uint32 *ecx, uint32 *edx, uint32 *eax);

void ASM_F sti();

void ASM_F cli();

void ASM_F hlt();

uint64 ASM_F read_cr8();

void ASM_F write_cr8(uint64 val);

uint64 ASM_F read_cr3();

void ASM_F write_cr3(uint64 val);

void ASM_F flush_tlb();

#define BOCHS_BREAK __asm__("xchg %bx,%bx");
