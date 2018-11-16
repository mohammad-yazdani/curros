/*
 * This code is a direct copy of OSDev tutorial: https://wiki.osdev.org/ELF_Tutorial
 * 	- Mohammad Yazdani
 * */


#include <stdint.h>

# define ELF_NIDENT	16


// TODO : Change to built in types in case of --nostdlib

typedef uint16_t Elf32_Half;	// Unsigned half int
typedef uint32_t Elf32_Off;	// Unsigned offset
typedef uint32_t Elf32_Addr;	// Unsigned address
typedef uint32_t Elf32_Word;	// Unsigned int
typedef int32_t  Elf32_Sword;	// Signed int

typedef struct {
	uint8_t		e_ident[ELF_NIDENT];
	Elf32_Half	e_type;
	Elf32_Half	e_machine;
	Elf32_Word	e_version;
	Elf32_Addr	e_entry;
	Elf32_Off	e_phoff;
	Elf32_Off	e_shoff;
	Elf32_Word	e_flags;
	Elf32_Half	e_ehsize;
	Elf32_Half	e_phentsize;
	Elf32_Half	e_phnum;
	Elf32_Half	e_shentsize;
	Elf32_Half	e_shnum;
	Elf32_Half	e_shstrndx;
} Elf32_Ehdr;

enum Elf_Ident {
	EI_MAG0		= 0, // 0x7F
	EI_MAG1		= 1, // 'E'
	EI_MAG2		= 2, // 'L'
	EI_MAG3		= 3, // 'F'
	EI_CLASS	= 4, // Architecture (32/64)
	EI_DATA		= 5, // Byte Order
	EI_VERSION	= 6, // ELF Version
	EI_OSABI	= 7, // OS Specific
	EI_ABIVERSION	= 8, // OS Specific
	EI_PAD		= 9  // Padding
};

# define ELFMAG0	0x7F // e_ident[EI_MAG0]
# define ELFMAG1	'E'  // e_ident[EI_MAG1]
# define ELFMAG2	'L'  // e_ident[EI_MAG2]
# define ELFMAG3	'F'  // e_ident[EI_MAG3]

# define ELFDATA2LSB	(1)  // Little Endian
# define ELFCLASS32	(1)  // 32-bit Architecture

enum Elf_Type {
	ET_NONE		= 0, // Unkown Type
	ET_REL		= 1, // Relocatable File
	ET_EXEC		= 2  // Executable File
};

# define EM_386		(3)  // x86 Machine Type
# define EV_CURRENT	(1)  // ELF Current Version

// Helpers
bool elf_check_file(Elf32_Ehdr *hdr) {
	if(!hdr) return false;
	if(hdr->e_ident[EI_MAG0] != ELFMAG0) {
		ERROR("ELF Header EI_MAG0 incorrect.\n");
		return false;
	}
	if(hdr->e_ident[EI_MAG1] != ELFMAG1) {
		ERROR("ELF Header EI_MAG1 incorrect.\n");
		return false;
	}
	if(hdr->e_ident[EI_MAG2] != ELFMAG2) {
		ERROR("ELF Header EI_MAG2 incorrect.\n");
		return false;
	}
	if(hdr->e_ident[EI_MAG3] != ELFMAG3) {
		ERROR("ELF Header EI_MAG3 incorrect.\n");
		return false;
	}
	return true;
}

bool elf_check_supported(Elf32_Ehdr *hdr) {
	if(!elf_check_file(hdr)) {
		ERROR("Invalid ELF File.\n");
		return false;
	}
	if(hdr->e_ident[EI_CLASS] != ELFCLASS32) {
		ERROR("Unsupported ELF File Class.\n");
		return false;
	}
	if(hdr->e_ident[EI_DATA] != ELFDATA2LSB) {
		ERROR("Unsupported ELF File byte order.\n");
		return false;
	}
	if(hdr->e_machine != EM_386) {
		ERROR("Unsupported ELF File target.\n");
		return false;
	}
	if(hdr->e_ident[EI_VERSION] != EV_CURRENT) {
		ERROR("Unsupported ELF File version.\n");
		return false;
	}
	if(hdr->e_type != ET_REL && hdr->e_type != ET_EXEC) {
		ERROR("Unsupported ELF File type.\n");
		return false;
	}
	return true;
}

static inline void *elf_load_rel(Elf32_Ehdr *hdr) {
	int result;
	result = elf_load_stage1(hdr);
	if(result == ELF_RELOC_ERR) {
		ERROR("Unable to load ELF file.\n");
		return NULL;
	}
	result = elf_load_stage2(hdr);
	if(result == ELF_RELOC_ERR) {
		ERROR("Unable to load ELF file.\n");
		return NULL;
	}
	// TODO : Parse the program header (if present)
	return (void *)hdr->e_entry;
}

void *elf_load_file(void *file) {
	Elf32_Ehdr *hdr = (Elf32_Ehdr *)file;
	if(!elf_check_supported(hdr)) {
		ERROR("ELF File cannot be loaded.\n");
		return;
	}
	switch(hdr->e_type) {
		case ET_EXEC:
			// TODO : Implement
			return NULL;
		case ET_REL:
			return elf_load_rel(hdr);
	}
	return NULL;
}

/* ------- SECTION HEADER ------- */

typedef struct {
	Elf32_Word	sh_name;
	Elf32_Word	sh_type;
	Elf32_Word	sh_flags;
	Elf32_Addr	sh_addr;
	Elf32_Off	sh_offset;
	Elf32_Word	sh_size;
	Elf32_Word	sh_link;
	Elf32_Word	sh_info;
	Elf32_Word	sh_addralign;
	Elf32_Word	sh_entsize;
} Elf32_Shdr;

# define SHN_UNDEF	(0x00) // Undefined/Not present

enum ShT_Types {
	SHT_NULL	= 0,   // Null section
	SHT_PROGBITS	= 1,   // Program information
	SHT_SYMTAB	= 2,   // Symbol table
	SHT_STRTAB	= 3,   // String table
	SHT_RELA	= 4,   // Relocation (w/ addend)
	SHT_NOBITS	= 8,   // Not present in file
	SHT_REL		= 9,   // Relocation (no addend)
};

enum ShT_Attributes {
	SHF_WRITE	= 0x01, // Writable section
	SHF_ALLOC	= 0x02  // Exists in memory
};

static inline Elf32_Shdr *elf_sheader(Elf32_Ehdr *hdr) {
	return (Elf32_Shdr *)((int)hdr + hdr->e_shoff);
}

static inline Elf32_Shdr *elf_section(Elf32_Ehdr *hdr, int idx) {
	return &elf_sheader(hdr)[idx];
}

static inline char *elf_str_table(Elf32_Ehdr *hdr) {
	if(hdr->e_shstrndx == SHN_UNDEF) return NULL;
	return (char *)hdr + elf_section(hdr, hdr->e_shstrndx)->sh_offset;
}
 
static inline char *elf_lookup_string(Elf32_Ehdr *hdr, int offset) {
	char *strtab = elf_str_table(hdr);
	if(strtab == NULL) return NULL;
	return strtab + offset;
}

/* ------- SYMBOL TABLE ------- */

typedef struct {
	Elf32_Word		st_name;
	Elf32_Addr		st_value;
	Elf32_Word		st_size;
	uint8_t			st_info;
	uint8_t			st_other;
	Elf32_Half		st_shndx;
} Elf32_Sym;

# define ELF32_ST_BIND(INFO)	((INFO) >> 4)
# define ELF32_ST_TYPE(INFO)	((INFO) & 0x0F)

enum StT_Bindings {
	STB_LOCAL		= 0, // Local scope
	STB_GLOBAL		= 1, // Global scope
	STB_WEAK		= 2  // Weak, (ie. __attribute__((weak)))
};

enum StT_Types {
	STT_NOTYPE		= 0, // No type
	STT_OBJECT		= 1, // Variables, arrays, etc.
	STT_FUNC		= 2  // Methods or functions
};

static int elf_get_symval(Elf32_Ehdr *hdr, int table, uint idx) {
	if(table == SHN_UNDEF || idx == SHN_UNDEF) return 0;
	Elf32_Shdr *symtab = elf_section(hdr, table);
 
	uint32_t symtab_entries = symtab->sh_size / symtab->sh_entsize;
	if(idx >= symtab_entries) {
		ERROR("Symbol Index out of Range (%d:%u).\n", table, idx);
		return ELF_RELOC_ERR;
	}
 
	int symaddr = (int)hdr + symtab->sh_offset;
	Elf32_Sym *symbol = &((Elf32_Sym *)symaddr)[idx];

	if(symbol->st_shndx == SHN_UNDEF) {
		// External symbol, lookup value
		Elf32_Shdr *strtab = elf_section(hdr, symtab->sh_link);
		const char *name = (const char *)hdr + strtab->sh_offset + symbol->st_name;

		extern void *elf_lookup_symbol(const char *name);
		void *target = elf_lookup_symbol(name);

		if(target == NULL) {
			// Extern symbol not found
			if(ELF32_ST_BIND(symbol->st_info) & STB_WEAK) {
				// Weak symbol initialized as 0
				return 0;
			} else {
				ERROR("Undefined External Symbol : %s.\n", name);
				return ELF_RELOC_ERR;
			}
		} else {
			return (int)target;
		}

		} else if(symbol->st_shndx == SHN_ABS) {
		// Absolute symbol
		return symbol->st_value;
	} else {
		// Internally defined symbol
		Elf32_Shdr *target = elf_section(hdr, symbol->st_shndx);
		return (int)hdr + symbol->st_value + target->sh_offset;
	}
}

/* ------- BSS ------- */
static int elf_load_stage1(Elf32_Ehdr *hdr) {
	Elf32_Shdr *shdr = elf_sheader(hdr);

	unsigned int i;
	// Iterate over section headers
	for(i = 0; i < hdr->e_shnum; i++) {
		Elf32_Shdr *section = &shdr[i];

		// If the section isn't present in the file
		if(section->sh_type == SHT_NOBITS) {
			// Skip if it the section is empty
			if(!section->sh_size) continue;
			// If the section should appear in memory
			if(section->sh_flags & SHF_ALLOC) {
				// Allocate and zero some memory
				void *mem = malloc(section->sh_size);
				memset(mem, 0, section->sh_size);

				// Assign the memory offset to the section offset
				section->sh_offset = (int)mem - (int)hdr;
				DEBUG("Allocated memory for a section (%ld).\n", section->sh_size);
			}
		}
	}
	return 0;
}

typedef struct {
	Elf32_Addr		r_offset;
	Elf32_Word		r_info;
} Elf32_Rel;

typedef struct {
	Elf32_Addr		r_offset;
	Elf32_Word		r_info;
	Elf32_Sword		r_addend;
} Elf32_Rela;

# define ELF32_R_SYM(INFO)	((INFO) >> 8)
# define ELF32_R_TYPE(INFO)	((uint8_t)(INFO))

enum RtT_Types {
	R_386_NONE		= 0, // No relocation
	R_386_32		= 1, // Symbol + Offset
	R_386_PC32		= 2  // Symbol + Offset - Section Offset
};

# define ELF_RELOC_ERR -1

static int elf_load_stage2(Elf32_Ehdr *hdr) {
	Elf32_Shdr *shdr = elf_sheader(hdr);

	unsigned int i, idx;
	// Iterate over section headers
	for(i = 0; i < hdr->e_shnum; i++) {
		Elf32_Shdr *section = &shdr[i];

		// If this is a relocation section
		if(section->sh_type == SHT_REL) {
			// Process each entry in the table
			for(idx = 0; idx < section->sh_size / section->sh_entsize; idx++) {
				Elf32_Rel *reltab = &((Elf32_Rel *)((int)hdr + section->sh_offset))[idx];
				int result = elf_do_reloc(hdr, reltab, section);
				// On error, display a message and return
				if(result == ELF_RELOC_ERR) {
					ERROR("Failed to relocate symbol.\n");
					return ELF_RELOC_ERR;
				}
			}
		}
	}
	return 0;
}

# define DO_386_32(S, A)	((S) + (A))
# define DO_386_PC32(S, A, P)	((S) + (A) - (P))

static int elf_do_reloc(Elf32_Ehdr *hdr, Elf32_Rel *rel, Elf32_Shdr *reltab) {
	Elf32_Shdr *target = elf_section(hdr, reltab->sh_info);

	int addr = (int)hdr + target->sh_offset;
	int *ref = (int *)(addr + rel->r_offset);

	// Symbol value
	int symval = 0;
	if(ELF32_R_SYM(rel->r_info) != SHN_UNDEF) {
		symval = elf_get_symval(hdr, reltab->sh_link, ELF32_R_SYM(rel->r_info));
		if(symval == ELF_RELOC_ERR) return ELF_RELOC_ERR;
	}

	// Relocate based on type
	switch(ELF32_R_TYPE(rel->r_info)) {
		case R_386_NONE:
			// No relocation
			break;
		case R_386_32:
			// Symbol + Offset
			*ref = DO_386_32(symval, *ref);
			break;
		case R_386_PC32:
			// Symbol + Offset - Section Offset
			*ref = DO_386_PC32(symval, *ref, (int)ref);
			break;
		default:
			// Relocation type not supported, display error and return
			ERROR("Unsupported Relocation Type (%d).\n", ELF32_R_TYPE(rel->r_info));
			return ELF_RELOC_ERR;
	}
	return symval;
}

/* ------- PROGRAM HEADER ------- */
typedef struct {
	Elf32_Word		p_type;
	Elf32_Off		p_offset;
	Elf32_Addr		p_vaddr;
	Elf32_Addr		p_paddr;
	Elf32_Word		p_filesz;
	Elf32_Word		p_memsz;
	Elf32_Word		p_flags;
	Elf32_Word		p_align;
} Elf32_Phdr;

