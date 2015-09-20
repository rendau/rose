#include "httpp.h"
#include "mem.h"
#include "objtypes.h"
#include "trace.h"
#include <stdlib.h>

#define HTTPP_CACHE_SIZE 60

static uint8_t inited = 0;
static uint16_t mc = 0;
static str_t buff = NULL;
static hmap_t bpkt = NULL;

int
httpp_init() {
  if(!inited) {
    buff = str_new();
    ASSERT(!buff, "str_new");
    str_2_byte(buff);

    bpkt = hmap_new();
    ASSERT(!bpkt, "hmap_new");

    inited = 1;
  }

  return 0;
 error:
  return -1;
}

httpp_t
httpp_new(void *ro, httpp_pkt_cbt cb) {
  httpp_t res;

  ASSERT(!inited, "httpp not initialized");

  if(!mc) {
    mc = mem_new_ot("httpp",
		    sizeof(struct httpp_st),
		    HTTPP_CACHE_SIZE, NULL);
    ASSERT(!mc, "mem_new_ot");
  }

  res = mem_alloc(mc);
  ASSERT(!res, "mem_alloc");

  res->obj.type = OBJ_OBJECT;
  res->ro = ro;
  res->cb = cb;
  res->data = str_new();
  ASSERT(!res->data, "str_new");
  res->hparsed = 0;
  res->clen = 0;
  res->pkt = hmap_new();
  ASSERT(!res->pkt, "hmap_new");

  return res;
 error:
  return NULL;
}

void
httpp_set_cb(httpp_t psr, httpp_pkt_cbt cb) {
  psr->cb = cb;
}

int
httpp_feed(httpp_t psr, char *ch, uint32_t chs) {
  hmap_t pkt;
  str_t data, hdr;
  char *end, *ptr1, *ptr2, *ptr3, *ptr4;
  int ret, tf;

  data = psr->data;
  pkt = psr->pkt;

  ret = str_add(data, ch, chs);
  ASSERT(ret, "str_add");

  while(data->l) {
    if(!psr->hparsed) {
      end = strstr(data->v, "\r\n\r\n");
      if(!end)
	break;
      *(end+2) = 0;

      ptr1 = data->v;
      while((ptr2 = strstr(ptr1, "\r\n"))) {
        *ptr2 = 0;
        if(ptr1 == data->v) { // first line
          tf = strncmp(ptr1, "HTTP", 4) ? 1 : 0;
          ptr3 = ptr1;
          ptr4 = strstr(ptr3, " ");
          if(ptr4) {
            hdr = str_new_wv(ptr3, ptr4-ptr3);
            ASSERT(!hdr, "str_new_wv");
            ret = hmap_set_val_ks(pkt, tf ? "method" : "version", OBJ(hdr));
            ASSERT(ret, "hmap_set_val_ks");

            ptr3 = ptr4 + 1;
            ptr4 = strstr(ptr3, " ");
            if(ptr4) {
              hdr = str_new_wv(ptr3, ptr4-ptr3);
              ASSERT(!hdr, "str_new_wv");
              ret = hmap_set_val_ks(pkt, tf ? "url" : "statusc", OBJ(hdr));
              ASSERT(ret, "hmap_set_val_ks");

              ptr3 = ptr4 + 1;
              hdr = str_new_wv(ptr3, 0);
              ASSERT(!hdr, "str_new_wv");
              ret = hmap_set_val_ks(pkt, tf ? "version" : "status", OBJ(hdr));
              ASSERT(ret, "hmap_set_val_ks");
            } else
              return 1;
          } else
            return 1;
        } else { // headers
          ptr3 = ptr1;
          ptr4 = strstr(ptr3, ": ");
          if(ptr4) {
            *ptr4 = 0;
            hdr = str_new_wv(ptr4+2, ptr2-ptr4-2);
            ASSERT(!hdr, "str_new_wv");
            ret = hmap_set_val_ks(pkt, ptr3, OBJ(hdr));
            ASSERT(ret, "hmap_set_val_ks");
            if(!strncmp(ptr3, "Content-Length", 14)) {
              psr->clen = atol(ptr4+2);
            }
            *ptr4 = ':';
          }
        }
        *ptr2 = '\r';
        ptr1 = ptr2 + 2;
      }

      *(end+2) = '\r';
      end += 4;
      str_reset(buff);
      if(end-data->v < data->l) {
        ret = str_set_val(buff, end, data->l-(end-data->v));
        ASSERT(ret, "str_set_val");
      }
      psr->data = buff;
      buff = data;
      data = psr->data;

      if(!psr->clen) {
        ret = hmap_set_val_ks_vs(pkt, "__content__", "");
        ASSERT(ret, "hmap_set_val_ks_vs");
	psr->pkt = bpkt;
	bpkt = pkt;
	pkt = psr->pkt;
        psr->hparsed = 0;
        psr->clen = 0;
	ret = psr->cb(psr->ro, bpkt);
	ASSERT(ret, "cb");
	hmap_reset(bpkt);
      } else {
        psr->hparsed = 1;
      }
    } else {
      ASSERT(!psr->clen, "clen is 0");
      if(data->l < psr->clen)
	break;
      ret = str_set_val(buff, data->v+psr->clen, data->l-psr->clen);
      ASSERT(ret, "str_set_val");
      str_cut(data, psr->clen);
      psr->data = buff;
      buff = data;
      data = psr->data;
      ret = hmap_set_val_ks_vs(pkt, "__content__", buff->v);
      ASSERT(ret, "hmap_set_val_ks_vs");
      psr->pkt = bpkt;
      bpkt = pkt;
      pkt = psr->pkt;
      psr->hparsed = 0;
      psr->clen = 0;
      ret = psr->cb(psr->ro, bpkt);
      ASSERT(ret, "cb");
      hmap_reset(bpkt);
    }
  }

  return 0;
 error:
  return -1;
}

void
httpp_reset(httpp_t psr) {
  str_reset(psr->data);
  psr->hparsed = 0;
  psr->clen = 0;
  hmap_reset(psr->pkt);
  hmap_reset(bpkt);
}

void
httpp_destroy(httpp_t psr) {
  if(psr) {
    OBJ_DESTROY(psr->data);
    OBJ_DESTROY(psr->pkt);
    mem_free(mc, psr, 0);
  }
}
