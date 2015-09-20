#ifndef HTTPP_HEADER
#define HTTPP_HEADER

#include "obj.h"
#include <inttypes.h>

typedef struct httpp_st *httpp_t;

typedef struct hmap_st *hmap_t;
typedef struct str_st *str_t;

typedef int (*httpp_pkt_cbt) (void *ro, hmap_t pkt);

struct httpp_st {
  struct obj_st obj;
  void *ro;
  httpp_pkt_cbt cb;
  str_t data;
  uint8_t hparsed;
  uint32_t clen;
  hmap_t pkt;
};

int
httpp_init();

httpp_t
httpp_new(void *ro, httpp_pkt_cbt cb);

void
httpp_set_cb(httpp_t psr, httpp_pkt_cbt cb);

int
httpp_feed(httpp_t psr, char *ch, uint32_t chs);

void
httpp_reset(httpp_t psr);

void
httpp_destroy(httpp_t psr);

#endif
