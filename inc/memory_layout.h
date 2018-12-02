/* System memory layout */
#pragma once

#define K_START     		0xFFFF800000000000
#define K_PMAP_VADDR        0xFFFF800000000000
#define K_DYNAMIC 		    0xFFFFFFFF00000000
#define K_DYN_END           0xFFFFFFFF70000000
#define K_IMAGE 		    0xFFFFFFFF80000000

#define PAGE_SIZE 			0x1000

#define R_PADDR(paddr) (void*)((paddr) + K_PMAP_VADDR)
#define IS_KERN_SPACE(vaddr) (((uintptr)vaddr) < K_START)
