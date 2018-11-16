global enter_kernel
enter_kernel:
	push	ebp
	mov	ebp, esp                ; Set up the stack so the variables passed from the C code can be read
 
	mov	esi, [ebp+8]            ; This is the kernel entry point
	mov	[k_ptr], esi
 
	lgdt	[GDT.pointer]           ; Load GDT
 
	mov	ax, GDT.data            ; Reload data segment selectors
	mov	ss, ax
	mov	ds, ax
	mov	es, ax
	jmp	GDT.code:.jmp_k         ; Reload code selector by jumping to 64-bit code
.jmp_k:
	mov	edi, [ebp + 12]		; 1st argument of kernel_main (pointer to multiboot structure)
	mov	eax, [k_ptr]                                                                                                      ; This is transformed to mov rax, [k_ptr] and uses the double word reserved below 
	dd	0			; Trick the processor, contains high address of k_ptr                                     ; as higher half of the address to k_ptr
	jmp	eax                     ; This part is plain bad, tricking the processor is not the best thing to do here
 
section .data
k_ptr:
	dd	0

