_start:
	cli               				; Pause interrupts 
	mov	esp, stack_top          	; Set the loader_main stack
    mov     ebp, stack_top
 
	push	ebx                     ; Push pointer to multiboot info structure
	call	lmain					; Call loader_main fn	

