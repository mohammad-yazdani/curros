
clang --target=x86_64-pc-none-elf -ffreestanding -fno-builtin -nostdlib -nostdinc -c ./kmain.c -o ./build/kmain.o
ld.lld -T ./boot/link.ld build/kmain.o -o ./build/sys.bin
