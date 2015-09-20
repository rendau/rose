#include "wsp.h"
#include "mem.h"
#include "crypt.h"
#include "objtypes.h"
#include "trace.h"
#include "ns.h"
#include <stdlib.h>

#define WSP_CACHE_SIZE 60

static uint8_t inited = 0;
static uint16_t mc = 0;
static char header[10];
static str_t buff = NULL;

int
wsp_init() {
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

wsp_t
wsp_new(void *ro, wsp_pkt_cbt cb) {
  wsp_t res;

  ASSERT(!inited, "wsp not initialized");

  if(!mc) {
    mc = mem_new_ot("wsp",
		    sizeof(struct wsp_st),
		    WSP_CACHE_SIZE, NULL);
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
  res->fdata = str_new();
  ASSERT(!res->fdata, "str_new");
  str_2_byte(res->fdata);

  return res;
 error:
  return NULL;
}

void
wsp_set_cb(wsp_t psr, wsp_pkt_cbt cb) {
  psr->cb = cb;
}

int
wsp_get_accept_key(str_t key) {
  char *sha1_h;
  int ret;

  ret = str_add(key, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 36);
  ASSERT(ret, "str_add");

  sha1_h = (char *)crypt_sha1_enc(key->v, key->l);

  str_reset(key);

  ret = crypt_base64_enc(key, sha1_h, CRYPT_SHA1_DS);
  ASSERT(ret, "crypt_base64_enc");

  return 0;
 error:
  return -1;
}

int
wsp_feed(wsp_t psr, char *ch, uint32_t chs) {
  char *ind;
  str_t data;
  uint32_t need, i;
  unsigned char byte;
  int ret;

  data = psr->data;

  ret = str_add(data, ch, chs);
  ASSERT(ret, "str_add");

  need = 0;
  ind = data->v;
  while(ind-data->v < data->l) {
    if(!psr->hparsed) {
      if(data->l < 2)
        break;
      psr->b1 = data->v[0];
      psr->b2 = data->v[1];
      ind = data->v+2;
      byte = psr->b2 & (~0x80);
      if(!need) {
        need = (psr->b2 & 0x80) ? 4 : 0;
        if(byte < 0x7e) {
          need += 0;
        } else if(byte == 0x7e) {
          need += 2;
        } else if(byte == 0x7f) {
          need += 8;
        } else {
          PERR("ws-parse fail(wrong second byte)");
          goto error;
        }
      } else {
        if(data->l < need + 2)
          break;
        if(byte < 0x7e) {
          psr->psize = byte;
        } else if(byte == 0x7e) {
          psr->psize = ns_csu16(*((uint16_t *)ind));
          ind += 2;
        } else if(byte == 0x7f) {
          psr->psize = ns_csu64(*((uint64_t *)ind));
          ind += 8;
        }
        if(psr->b2 & 0x80) {
          psr->mask[0] = ind[0];
          psr->mask[1] = ind[1];
          psr->mask[2] = ind[2];
          psr->mask[3] = ind[3];
          ind += 4;
        }
        ret = str_set_val(buff, ind, data->l-(ind-data->v));
        ASSERT(ret, "str_set_val");
	psr->data = buff;
	buff = data;
        data = psr->data;
        ind = data->v;
        psr->hparsed = 1;
        need = 0;
      }
    } else {
      if(data->l < psr->psize)
        break;
      str_reset(buff);
      if(data->l > psr->psize) {
        ret = str_set_val(buff, data->v+psr->psize, data->l-psr->psize);
        ASSERT(ret, "str_set_val");
        str_cut(data, psr->psize);
      }
      psr->data = buff;
      buff = data;
      data = psr->data;
      ind = data->v;
      if(psr->b2 & 0x80) {
	for(i=0; i<buff->l; i++)
	  buff->v[i] ^= *(psr->mask + (i % 4));
      }
      psr->hparsed = 0;
      psr->psize = 0;
      if(!psr->fdata->l && (psr->b1 & 0x80)) {
	ret = psr->cb(psr->ro, psr->b1 & (~0x80), psr->b2, buff);
	ASSERT(ret, "cb");
      } else {
	if(!psr->fdata->l) {
	  psr->fb1 = psr->b1 & (~0x80);
	  psr->fb2 = psr->b2;
	}
	ret = str_add(psr->fdata, buff->v, buff->l);
	ASSERT(ret, "str_add");
	if(psr->b1 & 0x80) {
	  ret = psr->cb(psr->ro, psr->fb1, psr->fb2, psr->fdata);
	  ASSERT(ret, "cb");
	  str_reset(psr->fdata);
	}
      }
    }
  }

  return 0;
 error:
  return -1;
}

int
wsp_get_header(char **ptr, uint64_t dsize) {
  uint16_t size;

  *ptr = header;
  if(dsize < 126) {
    header[0] = 0x82;
    header[1] = dsize;
    return 2;
  } else if(dsize >= 126 && dsize <= 65535) {
    header[0] = 0x82;
    header[1] = 0x7e;
    size = ns_csu16(dsize);
    memcpy(header+2, &size, 2);
    return 4;
  } else {
    header[0] = 0x82;
    header[1] = 0x7f;
    dsize = ns_csu64(dsize);
    memcpy(header+2, &dsize, 8);
    return 10;
  }
}

void
wsp_destroy(wsp_t psr) {
  if(psr) {
    OBJ_DESTROY(psr->data);
    OBJ_DESTROY(psr->fdata);
    mem_free(mc, psr, 0);
  }
}
