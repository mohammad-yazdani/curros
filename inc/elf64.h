/* ELF64 Object representation and parsing header. */

#include "cdef.h"

typedef uint64 Elf64_Addr;
typedef uint64 Elf64_Off;
typedef uint16 Elf64_Half;
typedef uint32 Elf64_Word;
typedef uint32 Elf64_Sword;
typedef uint64 Elf64_Xword;
typedef uint64 Elf64_Sxword;
// This is custom
typedef uint8 uchar;


/* HEADER */

typedef struct ELF64_Header
{
    uchar e_ident[16];
    Elf64_Half e_type;
    Elf64_Half e_machine;
    Elf64_Word e_version;
    Elf64_Addr e_entry;
    Elf64_Off e_phoff;
    Elf64_Off e_shoff;
    Elf64_Word e_flags;
    Elf64_Half e_ehsize;
    Elf64_Half e_phentsize;
    Elf64_Half e_phnum;
    Elf64_Half e_shentsize;
    Elf64_Half e_shnum;
    Elf64_Half e_shstrndx;
} Elf64_Ehdr;

#define EI_MAG0 0 // should be \x7f
#define EI_MAG1 1 // E
#define EI_MAG2 2 // L
#define EI_MAG3 3 // F

#define ELFMAG0 '\x7f'
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6
#define EI_OSABI 7
#define EI_ABIVERSION 8
#define EI_PAD 9
#define EI_NIDENT 16

#define ELFCLASS32 1
#define ELFCLASS64 2

#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

#define ELFOSABI_SYSV 0
#define ELFOSABI_HPUX 1
#define ELFOSABI_STANDALONE 255

#define ET_NONE 0
#define ET_REL 1
#define ET_EXEC 2
#define ET_DYN 3
#define ET_CORE 4

#define ET_LOOS 0xFE00
#define ET_HIOS 0XFEFF
#define ET_LOPROC 0XFF00
#define ET_HIPROC 0XFFFF

#define EV_CURRENT 1

/* SECTIONS */

#define SHN_UNDEF 0
#define SHN_LOPROC 0xFF00
#define SHN_HIPROC 0XFF1F
#define SHN_LOOS 0XFF20
#define SHN_HIOS 0XFF3F
#define SHN_ABS 0XFFF1
#define SHN_COMMON 0XFFF2

typedef struct ELF64_Section_Header
{
    Elf64_Word sh_name;
    Elf64_Word sh_type;
    Elf64_Xword sh_flags;
    Elf64_Addr sh_addr;
    Elf64_Off sh_offset;
    Elf64_Xword sh_size;
    Elf64_Word sh_link;
    Elf64_Word sh_info;
    Elf64_Xword sh_addralign;
    Elf64_Xword sh_entsize;
} Elf64_Shdr;

#define SHT_NULL 0
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_RELA 4
#define SHT_HASH 5
#define SHT_DYNAMIC 6
#define SHT_NOTE 7
#define SHT_NOBITS 8
#define SHT_REL 9
#define SHT_SHLIB 10
#define SHT_DYNSYM 11

#define SHT_LOOS 0x60000000
#define SHT_HIOS 0x6FFFFFFF
#define SHT_LOPROC 0x70000000
#define SHT_HIPROC 0x7FFFFFFF

#define SHF_WRITE 0x1
#define SHF_ALLOC 0x2
#define SHF_EXECINSTR 0x4
#define SHF_MASKOS 0x0F000000
#define SHF_MASKPROC 0xF0000000





/* STRING TABLE */

typedef struct ELF_String_Table
{
    uchar * string;
} Elf64_Stbl;



/* SYMBOL TABLE */

typedef struct ELF64_Symbol_Table
{
    Elf64_Word st_name;
    uchar st_info;
    uchar st_other;
    Elf64_Half st_shndx;
    Elf64_Addr st_value;
    Elf64_Xword st_size;
} Elf64_Sym;

#define STB_LOCAL 0
#define STB_GLOBAL 1

#define STB_WEAK 2

#define STB_LOOS 10
#define STB_HIOS 12
#define STB_LOPROC 13
#define STB_HIPROC 15

#define STB_NOTYPE 0

#define STB_OBJECT 1
#define STB_FUNC 2
#define STB_SECTION 3
#define STB_FILE 4

#define STB_LOOS 10
#define STB_HIOS 12
#define STB_LOPROC 13
#define STB_HIPROC 15



/* RELOCATIONS */

/* PROGRAM HEADER TABLE */

/* NOTE SECTIONS */

/* DYNAMIC TABLE */

/* HASH TABLE */


/* FUNCTIONS */

void * elf_load_file(void * file);

