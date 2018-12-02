#include <multiboot2.h>
#include <pmm.h>
#include <cdef.h>


// Putting multiboot device info here
static char * loader_name = 0x0;
static void * acpi_ptr = 0x0;
static void * file_mod = 0x0;


uint64
process_reloc_load_addr(struct multiboot_tag_load_base_addr * tag)
{
	mheader * header = (mheader *) (tag->load_base_addr);
	if (header->magic == MULTIBOOT2_HEADER_MAGIC) {
		uint32 sum = header->magic;
		sum += header->architecture;
		sum += header->header_length;
		sum += header->checksum;
		if (sum == 0) {
			return (uint32) header;
		}
	}
	return 0;
}

void
process_mmap(struct multiboot_tag_mmap * mmap)
{
	struct multiboot_mmap_entry entry;
	uint16 num_entries = (mmap->size - sizeof(*mmap)) / mmap->entry_size;
	for (uint16 i = 0; i < num_entries; i++) {
		entry = mmap->entries[i];
		if (entry.type == 1) {
			if (entry.addr) set_mmap_high(entry.addr, entry.len);
			else set_mmap_low(entry.addr, entry.len);
		}
	}
}


void
process_tag(htag * tag)
{
	uint64 tag_addr = 0;

	struct multiboot_tag_mmap * mmap_tag = 0x0;
	struct multiboot_header_tag_module_align * mod_align = 0x0;
	struct multiboot_tag_load_base_addr * base_tag = 0x0;
	struct multiboot_tag_string * strtag = 0x0;
	struct multiboot_tag_old_acpi * acpi = 0x0;

	switch (tag->type) {
		case 0:
			if (tag->size == 8) return;
			tag_addr = (uint64) tag;
			tag_addr += sizeof(htag);
			process_tag((htag *) tag_addr);
			break;
		case 1:
			tag_addr = (uint64) tag;
			tag_addr += 16;
			process_tag((htag *) tag_addr);
			break;
		case 2:
			strtag = (struct multiboot_tag_string *) tag;
			loader_name = strtag->string;

			tag_addr = (uint64) strtag;
			tag_addr += 32;
			process_tag((htag *) tag_addr);
			break;
		case 3:
			file_mod = tag;
			tag_addr = (uint64) tag;
			tag_addr += 24;
			process_tag((htag *) tag_addr);
			break;
		case 4:
			tag_addr = (uint64) tag;
			tag_addr += tag->size;
			process_tag((htag *) tag_addr);
			break;
		case 5:
			tag_addr = (uint64) tag;
			tag_addr += 24;
			process_tag((htag *) tag_addr);
			break;
		case 6:
			mmap_tag = (struct multiboot_tag_mmap *) tag;
			mod_align = (struct multiboot_header_tag_module_align *) tag;
			process_mmap(mmap_tag);
			tag_addr = (uint64) mmap_tag;
			tag_addr += mmap_tag->size;
			process_tag((htag *) tag_addr);
			break;
		case 7:
			tag_addr = (uint64) tag;
			tag_addr += tag->size;
			process_tag((htag *) tag_addr);
			break;
		case 8:
			tag_addr = (uint64) tag;
			tag_addr += 40;
			process_tag((htag *) tag_addr);
			break;
		case 9:
			tag_addr = (uint64) tag;
			tag_addr += 1304;
			process_tag((htag *) tag_addr);
			break;
		case 10:
			tag_addr = (uint64) tag;
			tag_addr += 32;
			process_tag((htag *) tag_addr);
			break;
		case 14:
			acpi = (struct multiboot_tag_old_acpi *) tag;
			acpi_ptr = acpi->rsdp;
			tag_addr = (uint64) tag;
			tag_addr += 32;
			process_tag((htag *) tag_addr);
			break;
		case 21:
			base_tag = (struct multiboot_tag_load_base_addr *) tag;
			tag_addr = process_reloc_load_addr(base_tag);
			if (tag_addr > 0) {
				tag_addr = (uint64) base_tag;
				tag_addr += 16;
				process_tag((htag *) tag_addr);
			}
			break;
		default:
			tag_addr = (uint64) tag;
			tag_addr += sizeof(htag);
			process_tag((htag *) tag_addr);
			break;
	}
}

void
parse_mb2(mbentry * mb, void ** module, char ** ld_name)
{
    uint64 tag_base0 = (uint64) mb;
	uint64 next_entry;
	uint64 last_entry;

	next_entry = tag_base0 + 8;
	last_entry = tag_base0 + mb->total_size - 8;

	htag * last_tag = (htag *) last_entry;
	if (!(last_tag->type == 0 && last_tag->size == 8)) return;

	htag * tag = (htag *) next_entry;
	process_tag(tag);

    *module = file_mod;
    *ld_name = loader_name;
}

