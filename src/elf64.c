#include <elf64.h>
#include <cdef.h>

bool 
elf_check_file(Elf64_Ehdr *hdr) 
{
	if (!hdr) return 0;
	if (hdr->e_ident[EI_MAG0] != ELFMAG0) return 0;
	if (hdr->e_ident[EI_MAG1] != ELFMAG1) return 0;
	if (hdr->e_ident[EI_MAG2] != ELFMAG2) return 0;
	if (hdr->e_ident[EI_MAG3] != ELFMAG3) return 0;

	return 1;
}

bool 
elf_check_supported(Elf64_Ehdr *hdr)
{
	if (!elf_check_file(hdr)) return 0;
	if (hdr->e_ident[EI_CLASS] != ELFCLASS64) return 0;
	if (hdr->e_ident[EI_DATA] != ELFDATA2LSB) return 0; // TODO : May not be true
	//if (hdr->e_machine != EM_386) return 0; // TODO : CHECK
	if (hdr->e_ident[EI_VERSION] != EV_CURRENT) return 0;
	if (hdr->e_type != ET_REL && hdr->e_type != ET_EXEC) return 0;
	
    return 1;
}

static inline void *
elf_load_rel(Elf64_Ehdr * hdr)
{
    (void) hdr;
	/*int result;
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
    */
   return NULL; // TODO : IMPLEMENT IF NEEDED
}
 
void *
elf_load_file(void * file)
{
	Elf64_Ehdr * hdr = (Elf64_Ehdr *) file;
	if (!elf_check_supported(hdr)) return NULL;
	switch(hdr->e_type) {
		case ET_EXEC:
			return (void *) hdr->e_entry; // TODO : Implement
		case ET_REL:
			return elf_load_rel(hdr);
	}
	return NULL;
}
