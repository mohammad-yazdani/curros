/* System memory layout */

/**
 * Kernel Memory Layout
 * ----------------------- 0x0000,0000,0000,0000 - User Space
 * Application       SIZE: 0x0000,8000,0000,0000 (256x PML4)
 * ----------------------- 0x0000,7FFF,FFFF,FFFF
 * Non-canonical
 * ----------------------- 0xFFFF,8000,0000,0000 - Kernel Space
 * Phys mem identity map   0x0000,0100,0000,0000 (2x PML4) 
 * ----------------------- 0xFFFF,8100,0000,0000
 * Mem mapped IO           0x0000,0100,0000,0000 (2x PML4)
 * ----------------------- 0xFFFF,8200,0000,0000
 * Kernel dynamic region   
 * ----------------------- 0xFFFF,FFFF,8000,0000 (-2GB)
 * Kernel Image
 * ----------------------- 0xFFFF,FFFF,FFFF,FFFF
**/

#define KERNEL_SPACE_VADDR        (0xFFFF800000000000)
#define KERNEL_IMAGE_PADDR        (0x1000000)
#define KERNEL_IMAGE_VADDR        (0xFFFFFFFF80000000)

#define KERNEL_IMAGE_OFFSET       (KERNEL_IMAGE_PADDR)
#define KERNEL_DYNAMIC_VADDR      (0xFFFF820000000000)
#define KERNEL_PHYS_MEM_MAP_VADDR (0xFFFF800000000000)
#define KERNEL_PAGE_SIZE          (0x1000)


