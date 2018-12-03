#include <elf64.h>
#include <cdef.h>
#include <error.h>
#include <paging.h>
#include <pmm.h>
#include <print.h>
#include <clib.h>

bool
elf_check_file(elf64_hdr *hdr)
{
    if (!hdr)
    { return 0; }
    if (hdr->e_ident[EI_MAG0] != ELFMAG0)
    { return 0; }
    if (hdr->e_ident[EI_MAG1] != ELFMAG1)
    { return 0; }
    if (hdr->e_ident[EI_MAG2] != ELFMAG2)
    { return 0; }
    if (hdr->e_ident[EI_MAG3] != ELFMAG3)
    { return 0; }

    return 1;
}

bool
elf_check_supported(elf64_hdr *hdr)
{
    if (!elf_check_file(hdr))
    { return 0; }
    if (hdr->e_ident[EI_CLASS] != ELFCLASS64)
    { return 0; }
    if (hdr->e_ident[EI_DATA] != ELFDATA2LSB)
    { return 0; } // TODO : May not be true
    //if (hdr->e_machine != EM_386) return 0; // TODO : CHECK
    if (hdr->e_ident[EI_VERSION] != EV_CURRENT)
    { return 0; }
    if (hdr->e_type != ET_EXEC)
    { return 0; }

    return 1;
}

static int32 elf_load_seg(struct pcb *proc, void *seg_start, usize seg_size, void *vaddr)
{
    int32 ret = ESUCCESS;
    if (((uintptr) vaddr & 0xfff) != 0)
    {
        // alignment
        ret = EINVARG;
    }

    if (ret == ESUCCESS)
    {
        uint32 num_pages = ((uint32) seg_size + KERNEL_PAGE_SIZE - 1) / KERNEL_PAGE_SIZE;

        // allocate memory
        for (uint32 i = 0; i < num_pages; i++)
        {
            // map pages
            uintptr page = (uintptr) pmalloc(KERNEL_PAGE_SIZE);
            if (page == (uintptr) NULL)
            {
                ret = ENOMEM;
            }

            if (ret == ESUCCESS)
            {
                ret = map_vmem(proc->cr3, (uintptr) vaddr + i * KERNEL_PAGE_SIZE, page);
            }

            if (ret != ESUCCESS)
            {
                break;
            }
        }

        // copy segment
        mem_cpy(seg_start, vaddr, seg_size);
    }

    if (ret != ESUCCESS)
    {
        // cleanup
    }

    return ret;
}

int32
elf_load_file(struct pcb *proc, void *file, void **entry)
{
    int32 ret = ESUCCESS;
    elf64_hdr *hdr = (elf64_hdr *) file;
    if (!elf_check_supported(hdr))
    {
        ret = EINVARG;
    }

    if (ret == ESUCCESS)
    {
        elf64_phdr *phdr_start = (elf64_phdr *) ((uintptr) file + hdr->e_phoff);
        // load program segments
        for (int i = 0; i < hdr->e_phnum; i++)
        {
            elf64_phdr *each_seg = (elf64_phdr *) (phdr_start + i * sizeof(elf64_phdr));
            if (each_seg->p_flags == PT_LOAD)
            {
                if (each_seg->p_memsz != each_seg->p_filesz)
                {
                    ret = EINVARG;
                }
                else
                {
                    kprintf("Loading segment offset 0x%x to vaddr 0x%x, size: %d", (uint64) each_seg->p_offset,
                            (uint64) each_seg->p_vaddr, (uint64) each_seg->p_memsz);
                    // load segment
                    ret = elf_load_seg(proc, (void *) ((uintptr) file + each_seg->p_offset), each_seg->p_memsz,
                                       (void *) each_seg->p_vaddr);
                }
            }
            if (ret != ESUCCESS)
            {
                break;
            }
        }
    }

    if (ret == ESUCCESS)
    {
        // write back entry
        *entry = (void *) hdr->e_entry;
    }

    return ret;
}
