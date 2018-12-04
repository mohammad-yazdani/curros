# CurrSOS
CurrSOS is a simple x86-64 kernel that supports interrupt, memory management, processes and threads, userspace and system calls. It has vastly simplier but not better memory management than its successor CurrOS.

## Toolchain
clang, lld, xorriso, grub-pc-bin, nasm

## Build
Run make inside "user" directory to build the test user program.
Run make in root dir and boot from out/curros.iso

## Clean
make clean
cd user 
make clean
