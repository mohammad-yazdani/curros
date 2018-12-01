#include "elf.h"

bool elf_check_file(Elf32_Ehdr *hdr) {
	if(!hdr) return 0;
	if(hdr->e_ident[EI_MAG0] != ELFMAG0) {
		return 0;
	}
	if(hdr->e_ident[EI_MAG1] != ELFMAG1) {
		return 0;
	}
	if(hdr->e_ident[EI_MAG2] != ELFMAG2) {
		return 0;
	}
	if(hdr->e_ident[EI_MAG3] != ELFMAG3) {
		return 0;
	}
	return 0;
}



