/*
 * This library is written based on the HP/Intel definition of the ELF-64 object file format.
 * Author: Mohammad Yazdani
 * */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "multiboot.h"

enum Elf_Ident {
	EI_MAG0			= 0,
	EI_MAG1			= 1,
	EI_MAG2			= 2,
	EI_MAG3			= 3,
	EI_CLASS		= 4, 		// Architecture (32/64)
	EI_DATA			= 5, 		// Byte Order
	EI_VERSION		= 6, 		// ELF Version
	EI_OSABI		= 7, 		// OS Specific
	EI_ABIVERSION 	= 8, 		// OS Specific
	EI_PAD			= 9,  		// Padding
	EI_NIDENT 		= 16		// Size of e_indent[]
};

// File type
# define ELFMAG0	0x7F 		// e_ident[EI_MAG0]
# define ELFMAG1	'E'  		// e_ident[EI_MAG1]
# define ELFMAG2	'L'  		// e_ident[EI_MAG2]
# define ELFMAG3	'F'  		// e_ident[EI_MAG3]

// Endianess
# define ELFDATA2LSB	(1)  	// Little Endian
# define ELFCLASS32		(2)  	// 32-bit Architecture

// ABI identifier
#define ELFOSABI_SYSV 		0
#define ELFOSABI_HPUX		1
#define ELFOSABI_STANDALONE 255

typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef int32_t Elf64_Sword;
typedef uint64_t Elf64_Xword;
typedef int64_t Elf64_Sxword;

typedef struct {
    unsigned char   e_indent[16];
    Elf64_Half      e_type;
    Elf64_Half      e_machine;
    Elf64_Word      e_version;
    Elf64_Addr      e_entry;
    Elf64_Off       e_phoff;
    Elf64_Off       e_shoff;
    Elf64_Word      e_flags;
    Elf64_Half      e_ehsize;
    Elf64_Half      e_phentsize;
    Elf64_Half      e_phnum;
    Elf64_Half      e_shentsize;
    Elf64_Half      e_shnum;
    Elf64_Half      e_shstrndx;

} Elf64_Ehdr;


enum Elf_Type {
	ET_NONE = 0,
	ET_REL 	= 1,
	ET_EXEC = 2
};

#define EM_386 		3
#define EV_CURRENT 	1


int
check_magic_num(const char * e_indent)
{
	int check = 0;

	check += (e_indent[EI_MAG0] == '\x7f');
	check += (e_indent[EI_MAG1] == 'E');
	check += (e_indent[EI_MAG2] == 'L');
	check += (e_indent[EI_MAG3] == 'F');

	return (check == 4);
}

// TODO : Implement elf_file_data_t
typedef elf_file_data_t;

// TODO : Implement elf_file_segment_t
typedef elf_file_segment_t;

// TODO : Implement parse_elf_executable
unsigned long parse_elf_executable(void *, unsigned long, elf_file_data_t*);

char* kernel_elf_space[sizeof(elf_file_data_t)];
elf_file_data_t* kernel_elf = (elf_file_data_t*) kernel_elf_space;

/* This function parses the ELF file and returns the entry point */
void* load_elf_module(multiboot_uint32_t mod_start, multiboot_uint32_t mod_end) {
	unsigned long err = parse_elf_executable((void*)mod_start, sizeof(elf_file_data_t), kernel_elf);    /* Parses ELF file and returns an error code */
	if(err == 0) {                                                                                       /* No errors occurred while parsing the file */
		for(int i = 0; i < kernel_elf->numSegments; i++) {
			elf_file_segment_t seg = kernel_elf->segments[i];                                   /* Load all the program segments into memory */
			/*  if you want to do relocation you should do so here, */
			const void* src = (const void*) (mod_start + seg.foffset);    /*  though that would require some changes to parse_elf_executable */
			memcpy((void*) seg.address, src, seg.flength);
		}
        return (void*) kernel_elf->entryAddr;                                                       /* Finally we can return the entry address */
    }
    return NULL;
}

