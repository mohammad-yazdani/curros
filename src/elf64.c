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

static int32 elf_load_seg(struct pcb *proc, void* file, elf64_phdr* hdr)
{
    void *seg_start = (void*)((uintptr)file + hdr->p_offset);
    void *vaddr = (void*)hdr->p_vaddr;
    usize file_size = hdr->p_filesz;
    usize mem_size = hdr->p_memsz;

    int32 ret = ESUCCESS;
    if (((uintptr) vaddr & 0xfff) != 0)
    {
        // alignment
        ret = EINVARG;
    }

    if (ret == ESUCCESS)
    {
        uint32 num_pages = ((uint32)mem_size + KERNEL_PAGE_SIZE - 1) / KERNEL_PAGE_SIZE;

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
                ret = map_vmem(proc->cr3, (uintptr) vaddr, page);
            }

            if (ret == ESUCCESS)
            {
                usize cpy_size = file_size > KERNEL_PAGE_SIZE ? KERNEL_PAGE_SIZE : (file_size % KERNEL_PAGE_SIZE);
                // copy the current segment
                mem_cpy(seg_start, R_PADDR(page), cpy_size);
            }

            if (ret != ESUCCESS)
            {
                break;
            }

            vaddr = (void *) ((uintptr) vaddr + KERNEL_PAGE_SIZE);
            seg_start = (void *) ((uintptr) seg_start + KERNEL_PAGE_SIZE);
        }
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
            elf64_phdr *each_seg = (elf64_phdr *) ((uintptr) phdr_start + i * hdr->e_phentsize);
            if (each_seg->p_type == PT_LOAD)
            {
                // load segment
                ret = elf_load_seg(proc, file, each_seg);
#ifdef KDBG
                if (ret == ESUCCESS)
                {
                    kprintf("Loaded segment offset 0x%x to vaddr 0x%x, fs: %d, ms: %d\n", (uint64) each_seg->p_offset,
                            (uint64) each_seg->p_vaddr, (uint64) each_seg->p_filesz, (uint64) each_seg->p_memsz);
                }
#endif
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
