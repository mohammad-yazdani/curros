#include "clib.h"
#include "print.h"
#include "intr.h"

void
mem_cpy(void *src, void *dst, uint64 size)
{
    char *cSrc = (char *) src;
    char *cDst = (char *) dst;
    while (size--)
    {
        *(cDst++) = *(cSrc++);
    }
}

void
mem_set(void *src, uint8 val, uint64 size)
{
    while (size--)
    {
        *(uint8 *) src = val;
        src = (void *) ((uintptr) src + 1);
    }
}

void
mem_mv(void *src, void *dst, uint64 size)
{
    if (src >= dst)
    {
        mem_cpy(src, dst, size);
        return;
    }
    src = (void *) ((uintptr) src + size - 1);
    dst = (void *) ((uintptr) dst + size - 1);
    while (size--)
    {
        *(char *) dst = *(char *) src;
        dst = (void *) ((uintptr) dst - 1);
        src = (void *) ((uintptr) src - 1);
    }
}

uint64
str_len(char const *str)
{
    uint64 length = 0;
    while (*str != 0)
    {
        str++;
        length++;
    }
    return length;
}

uint64
str_cmp(char const *str1, char const *str2)
{
    uint64 length = str_len(str1);
    if (length != str_len(str2))
    {
        return 0;
    }
    while (length--)
    {
        if (*(str1 + length) != *(str2 + length))
        {
            return 0;
        }
    }
    return 1;
}

void
poor_sleep(uint32 dat)
{
    for(uint32 i = 0; i < dat; i++)
    {
    }
}

void kassert_ex(const char *expr_str, const char *file, int32 line, int32 expr)
{
    if (!expr)
    {
        kprintf("Assertion \"%s\" failed at %s:%d.\n", expr_str, file, line);
        stop_cpu();
    }
}
