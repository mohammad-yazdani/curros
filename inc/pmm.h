/* Header file for physical memory management */

#include "types.h"

paddr_t pmalloc(void); // Allocate a physical page
void pfree(paddr_t paddr); // Free physical page

void zero_out(paddr_t base, ui64_t offset);

