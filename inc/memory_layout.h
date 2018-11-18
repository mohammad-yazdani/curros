/* System memory layout */

#define user_space 0x0
#define non_can 0x00007FFFFFFFFFFF
#define kernel_space 0xFFFF800000000000
#define mem_mapped_io 0xFFFF810000000000
#define kernel_dynamic 0xFFFFFFFF80000000
#define kernel_image 0xFFFFFFFFFFFFFFFF

#define PAGESZ 4096

