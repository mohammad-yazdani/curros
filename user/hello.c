#include <stdint.h>
#include <cdef.h>
#include <curros.h>

#define THREAD_DELAY (5000000*4)

void
main(void *dat)
{
    char *msg_ok = "[UThread3] Magic number OK.\n";
    char *msg_bad = "[UThread3] Magic number FAILED.\n";
    char msg[] = "[UThread3]  \n";

    uint32 success = 0;
    uint32 magic = 0x13141516;
    if ((uint32) dat == magic)
    {
        syscall(SYSCALL_FUNC_PRINT, msg_ok);
        success = 1;
    }
    else
    {
        syscall(SYSCALL_FUNC_PRINT, msg_bad);
        success = 0;
    }
    int i = 0;
    while (1)
    {
        poor_sleep(THREAD_DELAY);
        i = i % 10;
        msg[11] = i + 48;
        syscall(SYSCALL_FUNC_PRINT, msg);
        i++;
    }
    // don't return here, need to call thread exit
}
