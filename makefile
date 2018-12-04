
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
			-I$(INC)/ \
			$(C_FLAGS_$(MOD))

DUMP_FLAGS = -x86-asm-syntax=intel \
			 -disassemble \
			 -r \
			 -t \
			 -triple=x86_64-pc-none-elf \
			 -print-imm-hex

LD_FLAGS =  -fuse-ld=$(LD) \
			-nostdlib \
			-Wl, \
			-Wl,--fatal-warnings

# ===============================
# HERE COMES file definitions
# ===============================

SRC := .
OUT := out

# ===============================
# Add additional c source files here
# ===============================
C_SRC := hello_hell.c
TGT := $(OUT)/hello_hell.elf

# ===============================
# Compilation rules
# ===============================
C_OBJ := $(addsuffix .o, $(addprefix $(OUT)/,$(C_SRC)))

$(info $(C_OBJ))

$(C_OBJ): $(OUT)/%.c.o : $(SRC)/%.c
	$(CC) $(C_FLAGS) -o $@ $<

$(TGT): $(C_OBJ)
	$(CC) -o $@ $^ -v

$(DMP): $(TGT)
	$(DAS) $(DUMP_FLAGS) $< > $@



.PHONY: mkdir
mkdir:
	mkdir -p out

.PHONY: clean
clean:
	rm -rf $(OUT)

.PHONY: all
all: mkdir $(TGT) $(DMP)

