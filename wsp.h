#ifndef WSP_HEADER
#define WSP_HEADER

#include "obj.h"
#include <inttypes.h>

typedef struct wsp_st *wsp_t;

typedef struct str_st *str_t;

typedef int (*wsp_pkt_cbt) (void *, int, int, str_t);

struct wsp_st {
  struct obj_st obj;
  void *ro;
  wsp_pkt_cbt cb;
  str_t data;
  uint8_t hparsed;
  uint64_t psize;
  unsigned char b1;
  unsigned char b2;
  unsigned char mask[4];
  unsigned char fb1;
  unsigned char fb2;
  str_t fdata;
};

int
wsp_init();

wsp_t
wsp_new(void *ro, wsp_pkt_cbt cb);

void
wsp_set_cb(wsp_t psr, wsp_pkt_cbt cb);

int
wsp_get_accept_key(str_t key);

int
wsp_feed(wsp_t psr, char *ch, uint32_t chs);

int
wsp_get_header(char **ptr, uint64_t dsize);

void
wsp_destroy(wsp_t psr);

#endif
