#ifndef FRMP_HEADER
#define FRMP_HEADER

#include "obj.h"
#include <inttypes.h>

typedef struct frmp_st *frmp_t;

typedef struct str_st *str_t;

typedef int (*frmp_pkt_cbt) (void *, str_t);

struct frmp_st {
  struct obj_st obj;
  void *ro;
  frmp_pkt_cbt cb;
  str_t data;
  uint8_t hparsed;
  uint64_t psize;
};

int
frmp_init();

frmp_t
frmp_new(void *ro, frmp_pkt_cbt cb);

void
frmp_set_cb(frmp_t psr, frmp_pkt_cbt cb);

int
frmp_feed(frmp_t psr, char *ch, uint32_t chs);

int
frmp_get_header(char **ptr, uint64_t dsize);

void
frmp_reset(frmp_t psr);

void
frmp_destroy(frmp_t psr);

#endif
