#include "salloc.h"
#include "clib.h"

struct salloc_header
{
    union
    {
        uint32 size;
        uint32 flags;
    };
};

#define ALLOC_FLAG_NUM 2
#define ALLOC_HEADER_FLAG_FREE 0
#define ALLOC_HEADER_FLAG_LAST 1

static void
set_salloc_header_size(struct salloc_header *header, uint32 size)
{
    // align the integer, ignoring overflowed bits
    size <<= ALLOC_FLAG_NUM;

    // clear ALLOC_FLAG_NUM-th to 31-th bits
    header->size &= ~(uint32) bit_field_mask(ALLOC_FLAG_NUM, 31);
    // set bits
    header->size |= size;
}


static uint32
read_salloc_header_size(struct salloc_header *header)
{
    return header->size >> ALLOC_FLAG_NUM;
}


static uint32
read_salloc_header_flag(struct salloc_header *header, uint32 bit)
{
    return (header->flags & (uint32) bit_mask(bit)) == 0 ? 0 : 1;
}


static void
set_salloc_header_flag(struct salloc_header *header, uint32 bit, uint32 value)
{
    value &= (uint32) bit_mask(0);
    if (value == 1)
    {
        header->flags |= (uint32) bit_mask(bit);
    }
    else
    {
        header->flags &= ~(uint32) bit_mask(bit);
    }
}


static
void salloc_join(void *base)
{
    if (base != NULL)
    {
        char *c_ptr = (char *) base;
        while (1)
        {
            uint32 c_blk_free = read_salloc_header_flag((struct salloc_header *) c_ptr, ALLOC_HEADER_FLAG_FREE);
            uint32 c_blk_last = read_salloc_header_flag((struct salloc_header *) c_ptr, ALLOC_HEADER_FLAG_LAST);
            uint32 c_blk_size = read_salloc_header_size((struct salloc_header *) c_ptr);
            char *n_ptr = c_blk_last == 1 ? NULL : c_ptr + c_blk_size;
            if (n_ptr != NULL && c_blk_free == 1)
            {
                // if this is not the last block and the prev block is free
                uint32 n_blk_free = read_salloc_header_flag((struct salloc_header *) n_ptr, ALLOC_HEADER_FLAG_FREE);
                uint32 n_blk_last = read_salloc_header_flag((struct salloc_header *) n_ptr, ALLOC_HEADER_FLAG_LAST);
                uint32 n_blk_size = read_salloc_header_size((struct salloc_header *) n_ptr);

                if (n_blk_free == 1)
                {
                    // logically gone
                    set_salloc_header_size((struct salloc_header *) c_ptr, n_blk_size + c_blk_size);
                    set_salloc_header_flag((struct salloc_header *) c_ptr, ALLOC_HEADER_FLAG_LAST, n_blk_last);
                    continue;
                }
            }
            // update the c_ptr
            if (c_blk_last == 0)
            {
                c_ptr += c_blk_size;
            }
            else
            {
                break;
            }
        }
    }
}


bool
lb_salloc_assert(void *base, const uint32 *blk_size, const bool *blk_free, uint32 size)
{
    if (base == NULL || blk_free == NULL || blk_size == NULL)
    {
        return NULL;
    }
    uint32 i = 0;
    char *c_ptr = (char *) base;
    while (1)
    {
        uint32 cur_blk_free = read_salloc_header_flag((struct salloc_header *) c_ptr, ALLOC_HEADER_FLAG_FREE);
        uint32 cur_blk_last = read_salloc_header_flag((struct salloc_header *) c_ptr, ALLOC_HEADER_FLAG_LAST);
        uint32 cur_blk_size = read_salloc_header_size((struct salloc_header *) c_ptr);
        if (cur_blk_free != blk_free[i] || cur_blk_size != blk_size[i])
        {
            return FALSE;
        }
        else
        {
            c_ptr += cur_blk_size;
            i++;
        }
        if (cur_blk_last == 1)
        {
            return i == size;
        }
    }
}


void
lb_salloc_init(void *base, uint32 size)
{
    if (base != NULL && size >= sizeof(struct salloc_header))
    {
        struct salloc_header *ptr = (struct salloc_header *) base;
        set_salloc_header_size(ptr, size);
        set_salloc_header_flag(ptr, ALLOC_HEADER_FLAG_FREE, 1);
        set_salloc_header_flag(ptr, ALLOC_HEADER_FLAG_LAST, 1);
    }
}


void *
lb_salloc(void *base, uint32 size)
{
    void *result = NULL;
    if (base != NULL && size != 0)
    {
        uint32 total_size = size + sizeof(struct salloc_header);
        char *c_ptr = (char *) base;
        while (1)
        {
            uint32 cur_blk_free = read_salloc_header_flag((struct salloc_header *) c_ptr, ALLOC_HEADER_FLAG_FREE);
            uint32 cur_blk_size = read_salloc_header_size((struct salloc_header *) c_ptr);
            uint32 cur_blk_last = read_salloc_header_flag((struct salloc_header *) c_ptr, ALLOC_HEADER_FLAG_LAST);
            if (cur_blk_free == 0 || cur_blk_size < total_size)
            {
                //if cur block not a free block
                //or the current block size is less than the size we want
                if (cur_blk_last == 1)
                {
                    //if last one, break and fail.
                    break;
                }
                else
                {
                    c_ptr += cur_blk_size;
                }
            }
            else
            {
                // we have a free block with enough size
                if (total_size == cur_blk_size ||
                    cur_blk_size - total_size < sizeof(struct salloc_header))
                {
                    // since the space left is not enough for salloc_header
                    // we alloc the whole block
                    set_salloc_header_flag((struct salloc_header *) c_ptr, ALLOC_HEADER_FLAG_FREE, 0);
                }
                else
                {
                    // we split the block here
                    // set properties for the first block
                    set_salloc_header_size((struct salloc_header *) c_ptr, total_size);
                    set_salloc_header_flag((struct salloc_header *) c_ptr, ALLOC_HEADER_FLAG_LAST, 0);
                    set_salloc_header_flag((struct salloc_header *) c_ptr, ALLOC_HEADER_FLAG_FREE, 0);

                    // set properties for the second block
                    set_salloc_header_size((struct salloc_header *) (c_ptr + total_size), cur_blk_size - total_size);
                    set_salloc_header_flag((struct salloc_header *) (c_ptr + total_size), ALLOC_HEADER_FLAG_LAST,
                                           cur_blk_last);
                    set_salloc_header_flag((struct salloc_header *) (c_ptr + total_size), ALLOC_HEADER_FLAG_FREE, 1);
                }
                // return the pointer, skip the alloc header
                result = c_ptr + sizeof(struct salloc_header);
                break;
            }
        }
    }
    return result;
}


void
lb_sfree(void *base, void *ptr)
{
    if (base != NULL && ptr != NULL)
    {
        char *c_ptr = (char *) base;
        while (1)
        {
            uint32 cur_blk_free = read_salloc_header_flag((struct salloc_header *) c_ptr, ALLOC_HEADER_FLAG_FREE);
            uint32 cur_blk_last = read_salloc_header_flag((struct salloc_header *) c_ptr, ALLOC_HEADER_FLAG_LAST);
            uint32 cur_blk_size = read_salloc_header_size((struct salloc_header *) c_ptr);
            if (cur_blk_free == 0 && ptr == c_ptr + sizeof(struct salloc_header))
            {
                // we found the block, mark it as free
                set_salloc_header_flag((struct salloc_header *) c_ptr, ALLOC_HEADER_FLAG_FREE, 1);
                // merge blocks
                salloc_join(base);
                break;
            }

            if (cur_blk_last == 1)
            {
                break;
            }
            else
            {
                c_ptr += cur_blk_size;
            }
        }
    }
}
