# CurrOS
CurrOS is a simple x86-64 prototype kernel.
CurrOS provides basic OS functions such as interrupt handling, memory management, processes and threads, and userspace. We tested CurrOS on QEMU.

## Toolchain

clang, lld, xorriso, grub-pc-bin, nasm

## Emulation

### Qemu
Install qemu

Boot from out/curros.iso

### Bochs
Install bochs, bochs-x

Run "bochs -f bochsrc" at root dir

## Build
make

## Clean
make clean

## Paper 
https://github.com/mohammad-yazdani/curros/wiki/Project-Paper
