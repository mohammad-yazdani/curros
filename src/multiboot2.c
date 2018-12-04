#include <multiboot2.h>
#include <pmm.h>
#include <cdef.h>
#include <print.h>
#include <memory_layout.h>


// Putting multiboot device info here
static char *loader_name = 0x0;
static void *file_mod = 0x0;

// OH BOY DON"T I FUCKING LOVE THIS SHIT
static uint64 _low = 0;
static uint64 _high = 0;


void
process_mmap(struct multiboot_tag_mmap *mmap)
{
    struct multiboot_mmap_entry entry;

    uint16 num_entries = (mmap->size - sizeof(*mmap)) / mmap->entry_size;
    for (uint16 i = 0; i < num_entries; i++)
    {
        entry = mmap->entries[i];
        if ((entry.type == 1) && (entry.len > _high - _low))
        {
#ifdef KDBG
            kprintf("Update low: 0x%x high: 0x%x\n", (uint64) entry.addr, (uint64) (entry.addr + entry.len));
#endif
            _high = entry.addr + entry.len;
            _low = entry.addr;
        }
    }
}


void
process_tag(htag *tag)
{
    struct multiboot_tag_mmap *mmap_tag;
    struct multiboot_tag_string *strtag;
    struct multiboot_tag_module *modtag;
    while(1)
    {
        if(tag->type == MULTIBOOT_HEADER_TAG_END)
        {
            break;
        }

        switch (tag->type)
        {
            case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
                strtag = (struct multiboot_tag_string *) tag;
                loader_name = &strtag->string[0];
                tag = (void*)ALIGN(uintptr, (uintptr)tag + tag->size, MULTIBOOT_TAG_ALIGN);
                break;
            case MULTIBOOT_TAG_TYPE_MMAP:
                mmap_tag = (struct multiboot_tag_mmap *) tag;
                process_mmap(mmap_tag);
                tag = (void*)ALIGN(uintptr, (uintptr)tag + tag->size, MULTIBOOT_TAG_ALIGN);
                break;
            case MULTIBOOT_TAG_TYPE_MODULE:
                modtag = (struct multiboot_tag_module *)tag;
                file_mod = modtag;
                tag = (void*)ALIGN(uintptr, (uintptr)tag + tag->size, MULTIBOOT_TAG_ALIGN);
                break;
            default:
                tag = (void*)ALIGN(uintptr, (uintptr)tag + tag->size, MULTIBOOT_TAG_ALIGN);
                break;
        }
    }
}

void parse_mb2(mbentry *mb, void **module, char **ld_name, uint64 *low, uint64 *high)
{
    uint64 tag_base0 = (uint64) mb;
    uint64 next_entry;
    uint64 last_entry;

    next_entry = tag_base0 + 8;
    last_entry = tag_base0 + mb->total_size - 8;

    htag *last_tag = (htag *) last_entry;
    if (!(last_tag->type == 0 && last_tag->size == 8))
    { return; }

    htag *tag = (htag *) next_entry;
    process_tag(tag);

    *module = file_mod;
    *ld_name = loader_name;
    *low = _low;
    *high = _high;
}

