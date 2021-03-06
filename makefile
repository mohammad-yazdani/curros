AS := nasm
CC := clang-6.0
LD := lld-6.0
DAS := llvm-objdump-6.0

.DEFAULT_GOAL := all

C_FLAGS_ARCH_X86_64 := -mcmodel=kernel \
                  -target x86_64-pc-none-elf \
                  -mno-red-zone \
                  -mno-mmx \
                  -mno-sse \
                  -mno-sse2 \
                  -mno-sse3 \
                  -mno-3dnow

C_FLAGS =   -x c \
			-g \
            -c \
            -O0 \
			-std=c17 \
			-Wall \
			-Werror \
			-Wextra \
			-Wpedantic \
			-ffreestanding \
			-fno-pic \
			-fno-stack-protector \
			-Wno-int-to-pointer-cast \
			-Wno-zero-length-array \
			$(C_FLAGS_ARCH_X86_64) \
			-I$(INC)/ \
			$(C_EFLAGS)

AS_FLAGS =  -w+all \
			-w+error \
			-f elf64 \
			-F dwarf \
			-g \
			-I$(INC)/ \
			$(AS_FLAGS_$(MOD))

DUMP_FLAGS = -x86-asm-syntax=intel \
			 -disassemble \
			 -r \
			 -t \
			 -triple=x86_64-pc-none-elf \
			 -print-imm-hex

LD_FLAGS =  -fuse-ld=$(LD) \
			-nostdlib \
			-Wl,-T,$(LD_SCRIPT) \
			-Wl,--fatal-warnings

# ===============================
# HERE COMES file definitions
# ===============================

SRC := src
INC := inc
OUT := out
LD_SCRIPT := linker.ld
GRUB_CFG := grub.cfg

TGT := $(OUT)/kernel.elf
ISO := $(OUT)/curros.iso
DMP := $(OUT)/kernel.dmp

# ===============================
# Add additional c source files here
# ===============================
C_SRC := kmain.c \
	    llist.c \
		intr.c \
		clib.c \
		print.c \
		multiboot2.c \
		pmm.c \
		elf64.c \
		vmm.c \
		spin_lock.c \
		thread.c \
		proc.c \
		paging.c \
		syscall.c

# ===============================
# Add additional ASM source files here
# ===============================
ASM_SRC := boot.asm \
		   cpu.asm \
		   intr.asm


# ===============================
# Compilation rules
# ===============================
C_OBJ := $(addsuffix .o, $(addprefix $(OUT)/,$(C_SRC)))

ASM_OBJ := $(addsuffix .o, $(addprefix $(OUT)/,$(ASM_SRC)))

$(C_OBJ): $(OUT)/%.c.o : $(SRC)/%.c
	$(CC) $(C_FLAGS) -o $@ $<

$(ASM_OBJ): $(OUT)/%.asm.o : $(SRC)/%.asm
	$(AS) $(AS_FLAGS) -o $@ $<

$(TGT): $(C_OBJ) $(ASM_OBJ) $(LD_SCRIPT)
	$(CC) $(LD_FLAGS) -o $@ $^

$(DMP): $(TGT)
	$(DAS) $(DUMP_FLAGS) $< > $@

$(ISO): $(TGT) $(GRUB_CFG)
	mkdir -p $(OUT)/temp/boot/grub
	cp $(GRUB_CFG) $(OUT)/temp/boot/grub/
	cp $(TGT) $(OUT)/temp/
	cp ./user/out/hello.elf $(OUT)/temp/
	grub-mkrescue -d /usr/lib/grub/i386-pc -o $(ISO) $(OUT)/temp

.PHONY: mkdir
mkdir:
	mkdir -p out

.PHONY: clean
clean:
	rm -rf $(OUT)

.PHONY: all
all: mkdir $(TGT) $(DMP) $(ISO)

.PHONY: debug
debug: 
	qemu-system-x86_64 -boot d -cdrom $(ISO)

.PHONY: gdb
gdb: 
	qemu-system-x86_64 -s -m 128 -boot d -cdrom $(ISO) 

