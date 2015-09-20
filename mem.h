#ifndef MEM_HEADER
#define MEM_HEADER

#include <stdlib.h>
#include <inttypes.h>

typedef void (*mem_pre_free_ft)(void *ptr);

uint16_t
mem_new_ot(char *name, size_t size, uint16_t clim, mem_pre_free_ft pff);

void *
mem_alloc(uint16_t c);

void
mem_free(uint16_t c, void *ptr, int ff);

void
mem_set_sp();

void
mem_check();

#endif
