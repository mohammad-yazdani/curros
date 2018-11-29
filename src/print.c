#include "print.h"
#include "clib.h"

#define FBUF (0xb8000)
#define GET_ROW(pos) ((pos) / 80)
#define GET_POS(row, col) ((row) * 80 + (col))

static uint64 text_pos;

void print_init()
{
    text_pos = 0;
}

static void
print_scroll(void)
{
    mem_mv((void *) (FBUF + GET_POS(1, 0) * 2), (void *) (FBUF + GET_POS(0, 0) * 2), (80 * 24) * 2);
}

static void
print_str(char const *str)
{
    if (str == NULL)
    {
        return;
    }
    while (*str != 0)
    {
        if (*str == '\n')
        {
            text_pos = 80 * (GET_ROW(text_pos) + 1);
            if (text_pos > 80 * 25 - 1)
            {
                //can't hold
                print_scroll();
                mem_set((void *) (FBUF + 80 * 24 * 2), 0, 80 * 2); // clear last row
                text_pos = 80 * 24;
            }
            str++;
        }
        else
        {
            if (text_pos > 80 * 25 - 1)
            {
                //can't hold
                print_scroll();
                text_pos = 80 * 24;
            }
            *((char *) (FBUF) + text_pos * 2) = *str;
            *((char *) (FBUF) + text_pos * 2 + 1) = 7;
            str++;
            text_pos++;
        }
    }
}

static void
print_uint(uint64 number)
{
    char arr[21]; // do not need to initialize
    arr[20] = 0; //zero-terminated
    uint32 index = 19;
    uint32 const div = 10;
    while (1)
    {
        uint64 quo = number / div;
        uint64 rmd = number % div;
        number = quo;
        arr[index--] = (char) ('0' + rmd);
        if (number == 0)
        {
            break;
        }
    }
    print_str(&(arr[index + 1]));
}

static void
print_int(int64 number)
{
    char arr[21]; // do not need to initialize
    arr[20] = 0; //zero-terminated
    uint32 index = 19;
    uint32 isNegative = 0;
    uint32 const div = 10;
    if (number < 0)
    {
        isNegative = 1;
        number *= -1;
    }
    while (1)
    {
        int64 quo = number / div;
        int64 rmd = number % div;
        number = quo;
        arr[index--] = (char) ('0' + rmd);
        if (number == 0)
        {
            break;
        }
    }
    if (isNegative)
    {
        arr[index--] = '-';
    }
    print_str(&(arr[index + 1]));
}

static void
print_hex(uint64 number, uint64 capital)
{
    char const lookup_table_cap[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    char const lookup_table[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    char const *const look_up = capital == 1 ? &lookup_table_cap[0] : &lookup_table[0];
    char arr[17];
    arr[16] = 0; //zero-terminated
    uint32 index = 15;
    uint32 const div = 16;
    while (1)
    {
        uint64 quo = number / div;
        uint64 rmd = number % div;
        number = quo;
        arr[index--] = look_up[rmd];
        if (number == 0)
        {
            break;
        }
    }
    print_str(&(arr[index + 1]));
}

void
clear_screen(void)
{
    text_pos = 0; // reset text_pos
    mem_set((void *) FBUF, 0, 25 * 80 * 2);
}

static void
kvprintf(char const *format, va_list args)
{
    char buf[2];
    int64 d;
    uint64 u;
    char *s;

    buf[1] = '\0';
    for (; *format != '\0'; format++)
    {
        if (*format != '%')
        {
            buf[0] = *format;
            print_str(buf);
            continue;
        }
        format++;
        switch (*format)
        {
            case 'd':
                d = va_arg(args, int64);
                print_int(d);
                break;
            case 'u':
                u = va_arg(args, uint64);
                print_uint(u);
                break;
            case 's':
                s = va_arg(args, char *);
                print_str(s);
                break;
            case 'x':
                u = va_arg(args, uint64);
                print_hex(u, 0);
                break;
            case 'X':
                u = va_arg(args, uint64);
                print_hex(u, 1);
                break;
            case '%':
                buf[0] = '%';
                print_str(buf);
                break;
            default:
                buf[0] = '%';
                print_str(buf);
                format--;
                break;
        }
    }
}

void
kprintf(char const *format, ...)
{
    va_list args;
    va_start(args, format);
    kvprintf(format, args);
    va_end(args);
}
