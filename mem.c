#include "mem.h"
#include <string.h>
#include "trace.h"

#define MEM_OT_SIZE 100
#define MEM_CACHE_MAX 100

typedef struct ot_st *ot_t;

struct ot_st {
  char name[50];
  size_t osize;
  uint32_t ac;
  uint32_t floor;
  void *cache[MEM_CACHE_MAX];
  uint16_t c_len;
  uint16_t c_lim;
  mem_pre_free_ft pff;
};

static ot_t ots[MEM_OT_SIZE];
static uint16_t ots_len = 1;

uint16_t
mem_new_ot(char *name, size_t size, uint16_t clim, mem_pre_free_ft pff) {
  ot_t ot;

  ASSERT(ots_len==MEM_OT_SIZE, "ots count limit reached");

  ot = calloc(1, sizeof(struct ot_st));
  ASSERT(!ot, "calloc");

  strcpy(ot->name, name);
  ot->osize = size;
  ot->c_lim = (clim < MEM_CACHE_MAX) ? clim : (MEM_CACHE_MAX-1);
  ot->pff = pff;

  ots[ots_len++] = ot;

  return (ots_len-1);
 error:
  return 0;
}

void *
mem_alloc(uint16_t c) {
  ot_t ot;
  void *res;

  ot = ots[c];

  if(ot->c_len > 0) {
    res = ot->cache[--ot->c_len];
  } else {
    res = calloc(1, ot->osize);
    ASSERT(!res, "calloc");
  }
  ot->ac++;

  return res;
 error:
  return NULL;
}

void
mem_free(uint16_t c, void *ptr, int ff) {
  ot_t ot;

  if(ptr) {
    ot = ots[c];
    if(ot) {
      if((ot->c_len < ot->c_lim) && !ff) {
        ot->cache[ot->c_len++] = ptr;
      } else {
        if(ot->pff)
          ot->pff(ptr);
        free(ptr);
      }
      ot->ac--;
    }
  }
}

void
mem_set_sp() {
  uint16_t i;

  for(i=1; i<ots_len; i++)
    ots[i]->floor = ots[i]->ac;
}

void
mem_check() {
  ot_t ot;
  uint16_t i;

  for(i=1; i<ots_len; i++) {
    ot = ots[i];
    if(ot->ac - ot->floor) {
      PWAR("%d objects of '%s' type was not freed\n", ot->ac - ot->floor, ot->name);
    }
  }
}
