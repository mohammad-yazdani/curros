SYSCALL_VEC equ 51

SECTION .text
BITS 64

GLOBAL syscall

syscall:
; rdi = function number
; rsi = args
int SYSCALL_VEC
ret