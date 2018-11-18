ENTRY(sys_entry)

KERNEL_IMAGE_VADDR = 0xFFFFFFFF80000000;
KERNEL_IMAGE_PADDR = 0x1000000;
KERNEL_PAGE_SIZE = 0x1000;

SECTIONS
{
	. = KERNEL_IMAGE_VADDR + KERNEL_IMAGE_PADDR;

	.multiboot_header ALIGN(KERNEL_PAGE_SIZE) : AT(ADDR(.multiboot_header) - KERNEL_IMAGE_VADDR)
	{
		*(.multiboot_header)
	}

	.text ALIGN(KERNEL_PAGE_SIZE) : AT(ADDR(.text) - KERNEL_IMAGE_VADDR)
	{
		*(.text)
	}

	.data ALIGN(KERNEL_PAGE_SIZE) : AT(ADDR(.data) - KERNEL_IMAGE_VADDR)
	{
		*(.data)
		*(.rodata*)
	}

	.bss ALIGN(KERNEL_PAGE_SIZE) : AT(ADDR(.bss) - KERNEL_IMAGE_VADDR)
	{
		*(.bss)
		*(COMMON)
	}
}
