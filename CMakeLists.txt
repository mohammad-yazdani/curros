cmake_minimum_required(VERSION 3.12)
project(curros C)

set(CMAKE_C_STANDARD 11)

include_directories(boot)
include_directories(include)

add_executable(curros
        boot/elf.h
        boot/elf64.h
        boot/lmain.c
        boot/multiboot.h
        include/sys.h
        kmain.c)
