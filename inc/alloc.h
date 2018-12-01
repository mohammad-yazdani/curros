#pragma once

void *kalloc(usize size);

void free(void *addr);

#define GET_PHYS_MEM(x) (x)