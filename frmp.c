#include "frmp.h"
#include "mem.h"
#include "crypt.h"
#include "objtypes.h"
#include "trace.h"
#include "ns.h"
#include <stdlib.h>

#define FRMP_CACHE_SIZE 60

static uint8_t inited = 0;
static uint16_t mc = 0;
static char header[8];
static str_t buff = NULL;

int
frmp_init() {
  if(!inited) {
    buff = str_new();
    ASSERT(!buff, "str_new");
    str_2_byte(buff);

    inited = 1;
  }

  return 0;
 error:
  return -1;
}

frmp_t
frmp_new(void *ro, frmp_pkt_cbt cb) {
  frmp_t res;

  ASSERT(!inited, "wsp not initialized");

  if(!mc) {
    mc = mem_new_ot("frmp",
		    sizeof(struct frmp_st),
		    FRMP_CACHE_SIZE, NULL);
    ASSERT(!mc, "mem_new_ot");
  }

  res = mem_alloc(mc);
  ASSERT(!res, "mem_alloc");

  res->obj.type = OBJ_OBJECT;
  res->ro = ro;
  res->cb = cb;
  res->data = str_new();
  ASSERT(!res->data, "str_new");
  str_2_byte(res->data);
  res->hparsed = 0;
  res->psize = 0;

  return res;
 error:
  return NULL;
}

void
frmp_set_cb(frmp_t psr, frmp_pkt_cbt cb) {
  psr->cb = cb;
}

int
frmp_feed(frmp_t psr, char *ch, uint32_t chs) {
  char *ind;
  str_t data;
  int ret;

  data = psr->data;

  ret = str_add(data, ch, chs);
  ASSERT(ret, "str_add");

  ind = data->v;
  while(ind-data->v < data->l) {
    if(!psr->hparsed) {
      if(data->l < 8)
        break;
      psr->psize = ns_csu64(*((uint64_t *)ind));
      ind = data->v + 8;
      ret = str_set_val(buff, ind, data->l-(ind-data->v));
      ASSERT(ret, "str_set_val");
      psr->data = buff;
      buff = data;
      data = psr->data;
      ind = data->v;
      psr->hparsed = 1;
    } else {
      if(data->l < psr->psize)
        break;
      if(data->l > psr->psize) {
        ret = str_set_val(buff, data->v+psr->psize, data->l-psr->psize);
        ASSERT(ret, "str_set_val");
        str_cut(data, psr->psize);
	psr->data = buff;
	buff = data;
        data = psr->data;
        ind = data->v;
        psr->hparsed = 0;
        psr->psize = 0;
        ret = psr->cb(psr->ro, buff);
        ASSERT(ret, "cb");
      } else {
	str_reset(buff);
	psr->data = buff;
	buff = data;
        data = psr->data;
        ind = data->v;
        psr->hparsed = 0;
        psr->psize = 0;
        ret = psr->cb(psr->ro, buff);
        ASSERT(ret, "cb");
      }
    }
  }

  return 0;
 error:
  return -1;
}

int
frmp_get_header(char **ptr, uint64_t dsize) {
  dsize = ns_csu64(dsize);
  memcpy(header, &dsize, 8);
  *ptr = header;
  return 8;
}

void
frmp_reset(frmp_t psr) {
  str_reset(psr->data);
  psr->hparsed = 0;
  psr->psize = 0;
}

void
frmp_destroy(frmp_t psr) {
  if(psr) {
    OBJ_DESTROY(psr->data);
    mem_free(mc, psr, 0);
  }
}
