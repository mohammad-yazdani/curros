
AS := nasm
CC := clang-6.0
LD := lld-6.0
DAS := llvm-objdump-6.0

.DEFAULT_GOAL := all

C_FLAGS_ARCH_X86_64 := -target x86_64-pc-none-elf \
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
			-Wno-error=int-to-pointer-cast \
			-Wno-error=zero-length-array \
			$(C_FLAGS_ARCH_X86_64) \
			-I$(INC)/

AS_FLAGS =  -w+all \
			-w+error \
			-f elf64 \
			-F dwarf \
			-g \
			-I$(INC)/ \

LD_FLAGS =  -fuse-ld=$(LD) \
			-nostdlib \
			-Wl, linker.ld \
			-Wl,--fatal-warnings

# ===============================
# HERE COMES file definitions
# ===============================

INC := inc
SRC := .
OUT := out
TGT := $(OUT)/hello.elf

# ===============================
# Add additional c source files here
# ===============================
C_SRC := hello.c

# ===============================
# Add additional ASM source files here
# ===============================
ASM_SRC := curros.asm

# ===============================
# Compilation rules
# ===============================
C_OBJ := $(addsuffix .o, $(addprefix $(OUT)/,$(C_SRC)))

ASM_OBJ := $(addsuffix .o, $(addprefix $(OUT)/,$(ASM_SRC)))

$(C_OBJ): $(OUT)/%.c.o : %.c
	$(CC) $(C_FLAGS) -o $@ $<

$(ASM_OBJ): $(OUT)/%.asm.o : %.asm
	$(AS) $(AS_FLAGS) -o $@ $<

$(TGT): $(C_OBJ) $(ASM_OBJ)
	$(CC) $(LD_FLAGS) -o $@ $^

.PHONY: mkdir
mkdir:
	mkdir -p out

.PHONY: clean
clean:
	rm -rf $(OUT)

.PHONY: all
all: mkdir $(TGT)

