section .text
bits 64

global out_8
global out_16
global out_32
global in_8
global in_16
global in_32
global read_msr
global write_msr
global cpuid
global flush_gdt
global flush_idt
global cmp_xchg_32
global inc_32


out_32:
mov rdx,rdi
mov rax,rsi
out dx,eax
nop
nop
nop
ret


out_16:
mov rdx,rdi
mov rax,rsi
out dx,ax
nop
nop
nop
ret


out_8:
mov rdx,rdi
mov rax,rsi
out dx,al
nop
nop
nop
ret


in_8:
mov rdx,rdi
xor rax,rax
in al,dx
nop
nop
nop
ret

in_16:
mov rdx,rdi
xor rax,rax
in ax,dx
nop
nop
nop
ret


in_32:
mov rdx,rdi
xor rax,rax
in eax,dx
nop
nop
nop
ret

read_msr:
; preserve rdx
push rdx
mov ecx, dword [rdi]
rdmsr
mov dword [rdi], ecx
mov dword [rsi], edx
pop r11
mov dword [r11], eax
ret

write_msr:
mov ecx, dword [rdi]
mov eax, dword [rdx]
mov edx, dword [rsi]
wrmsr
ret


cpuid:
push rbp
mov rbp,rsp
; preserve rbx,rcx,rdx
push rbx
push rcx
push rdx
; cpuid parameters eax,ecx
mov eax, dword [rdi]
mov ecx, dword [rdx]
cpuid
; write results back to memory
mov dword [rdi], eax
mov dword [rsi], ebx
pop r11
mov dword [r11], ecx
pop r11
mov dword [r11], edx
pop rbx
mov rsp,rbp
pop rbp
ret

flush_gdt:
push rbp
mov rbp,rsp
lgdt [rdi]
;reload cs

push rdx ; data_slct : ss
push rbp ; rsp

pushfq
pop rax
push rax ; eflags

push rsi ; cs
mov rax, .reload
push rax ;rip
iretq
.reload:
mov es,dx
mov fs,dx
mov gs,dx
mov ds,dx
pop rbp
ret

flush_idt:
lidt [rdi]
ret

; ============================
cmpxchg_32:
mov eax, esi; eax = test_node_compare
lock cmpxchg dword [rdi], edx ; edx = val, rdi = ptr to dst
ret

; ============================
xinc_32:
lock xadd dword [rdi], esi ; [rdi] = [rdi] + esi, esi = old [rdi]
xor rax, rax
mov eax, esi
ret