ENTRY(main)

KERNEL_PAGE_SIZE = 0x1000;
USER_APPLICATION_VADDR = 0x1000000;

SECTIONS
{
	. = USER_APPLICATION_VADDR;

	.text ALIGN(KERNEL_PAGE_SIZE):
	{
		*(.text)
	}

	.data ALIGN(KERNEL_PAGE_SIZE):
	{
		*(.data)
		*(.rodata*)
	}

	.bss ALIGN(KERNEL_PAGE_SIZE):
	{
		*(.bss)
		*(COMMON)
	}
}
