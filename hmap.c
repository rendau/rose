#include "hmap.h"
#include "mem.h"
#include "objtypes.h"
#include "trace.h"
#include <stdlib.h>

#define HMAP_INIT_SIZE 20
#define HMAP_CACHE_SIZE 60

static int objp_size = sizeof(obj_t *);
static uint16_t mc = 0;

hmap_t
hmap_new() {
  hmap_t res;

  if(!mc) {
    mc = mem_new_ot("obj_hmap",
		    sizeof(struct hmap_st),
		    HMAP_CACHE_SIZE, hmap_pre_free_f);
    ASSERT(!mc, "mem_new_ot");
  }

  res = mem_alloc(mc);
  ASSERT(!res, "mem_alloc");

  if(!res->keys) {
    res->keys = calloc(HMAP_INIT_SIZE, objp_size);
    ASSERT(!res->keys, "calloc");
    res->vals = calloc(HMAP_INIT_SIZE, objp_size);
    ASSERT(!res->vals, "calloc");
    res->size = HMAP_INIT_SIZE;
  }

  res->obj.type = OBJ_HMAP;
  res->obj.copy_fun = hmap_new_wc;
  res->obj.print_fun = hmap_print;
  res->obj.destroy_fun = hmap_destroy;

  return res;
 error:
  return NULL;
}

obj_t
hmap_new_wc(obj_t obj) {
  obj_t k, v;
  hmap_t src, res;
  uint32_t i;
  int ret;

  ASSERT(!obj || (obj->type != OBJ_HMAP), "bad object");

  src = HMAP(obj);

  res = hmap_new();
  ASSERT(!res, "hmap_new");

  for(i=0; i<src->size; i++) {
    if(src->keys[i]) {
      k = v = NULL;
      if(src->keys[i]) {
        k = OBJ_COPY(src->keys[i]);
        ASSERT(!k, "OBJ_COPY");
      }
      if(src->vals[i]) {
        v = OBJ_COPY(src->vals[i]);
        ASSERT(!v, "OBJ_COPY");
      }
      ret = hmap_set_val(res, k, v);
      ASSERT(ret, "hmap_set_val");
    }
  }

  return OBJ(res);
 error:
  return NULL;
}

int
hmap_expand(hmap_t hmap) {
  void *ptr;

  ptr = realloc(hmap->keys, hmap->size * 2 * objp_size);
  ASSERT(!ptr, "realloc");
  hmap->keys = ptr;
  memset(hmap->keys + hmap->size, 0, hmap->size * objp_size);
  ptr = realloc(hmap->vals, hmap->size * 2 * objp_size);
  ASSERT(!ptr, "realloc");
  hmap->vals = ptr;
  memset(hmap->vals + hmap->size, 0, hmap->size * objp_size);
  hmap->size *= 2;

  return 0;
 error:
  return -1;
}

int
hmap_set_val(hmap_t hmap, obj_t k, obj_t v) {
  uint32_t i;

  for(i=0; i<hmap->size; i++) {
    if(!hmap->keys[i])
      break;
  }
  if(i == hmap->size)
    ASSERT(hmap_expand(hmap), "hmap_expand");
  hmap->keys[i] = k;
  hmap->vals[i] = v;
  hmap->len++;

  return 0;
 error:
  return -1;
}

int
hmap_set_val_ks(hmap_t hmap, char *k, obj_t v) {
  str_t key;
  uint32_t i;
  int ret;

  for(i=0; i<hmap->size; i++) {
    if(!hmap->keys[i]) continue;
    if(hmap->keys[i]->type != OBJ_STR) continue;
    if(!str_cmp_ws(STR(hmap->keys[i]), k))
      break;
  }
  if(i < hmap->size) {
    OBJ_DESTROY(hmap->vals[i]);
    hmap->vals[i] = v;
  } else {
    key = str_new_wv(k, 0);
    ASSERT(!key, "str_new_wv");
    ret = hmap_set_val(hmap, OBJ(key), v);
    ASSERT(ret, "hmap_set_val");
  }

  return 0;
 error:
  return -1;
}

int
hmap_set_val_ks_vs(hmap_t hmap, char *k, char *v) {
  str_t val;
  int ret;

  val = str_new_wv(v, 0);
  ASSERT(!val, "str_new_wv");

  ret = hmap_set_val_ks(hmap, k, OBJ(val));
  ASSERT(ret, "hmap_set_val_ks");

  return 0;
 error:
  return -1;
}

int
hmap_set_val_ks_vsc(hmap_t hmap, char *k, str_t v) {
  str_t val;
  int ret;

  val = STR(OBJ_COPY(v));
  ASSERT(!val, "OBJ_COPY");

  ret = hmap_set_val_ks(hmap, k, OBJ(val));
  ASSERT(ret, "hmap_set_val_ks");

  return 0;
 error:
  return -1;
}

int
hmap_set_val_ks_vb(hmap_t hmap, char *k, int v) {
  bool_t val;
  int ret;

  val = bool_new(v);
  ASSERT(!val, "bool_new");

  ret = hmap_set_val_ks(hmap, k, OBJ(val));
  ASSERT(ret, "hmap_set_val_ks");

  return 0;
 error:
  return -1;
}

int
hmap_set_val_ks_vi(hmap_t hmap, char *k, int64_t v) {
  int_t val;
  int ret;

  val = int_new(v);
  ASSERT(!val, "int_new");

  ret = hmap_set_val_ks(hmap, k, OBJ(val));
  ASSERT(ret, "hmap_set_val_ks");

  return 0;
 error:
  return -1;
}

int
hmap_set_val_ks_vf(hmap_t hmap, char *k, double v) {
  float_t val;
  int ret;

  val = float_new(v);
  ASSERT(!val, "float_new");

  ret = hmap_set_val_ks(hmap, k, OBJ(val));
  ASSERT(ret, "hmap_set_val_ks");

  return 0;
 error:
  return -1;
}

obj_t
hmap_get_val_ks(hmap_t hmap, char *k, int rk) {
  obj_t res;
  uint32_t i;

  res = NULL;
  for(i=0; i<hmap->size; i++) {
    if(!hmap->keys[i]) continue;
    if(hmap->keys[i]->type != OBJ_STR) continue;
    if(str_cmp_ws(STR(hmap->keys[i]), k)) continue;
    res = hmap->vals[i];
    if(rk) {
      OBJ_DESTROY(hmap->keys[i]);
      hmap->keys[i] = NULL;
      hmap->vals[i] = NULL;
      hmap->len--;
    }
    break;
  }

  return res;
}

void
hmap_remove_key_ks(hmap_t hmap, char *k) {
  uint32_t i;

  for(i=0; i<hmap->size; i++) {
    if(!hmap->keys[i]) continue;
    if(hmap->keys[i]->type != OBJ_STR) continue;
    if(str_cmp_ws(STR(hmap->keys[i]), k)) continue;
    OBJ_DESTROY(hmap->keys[i]);
    OBJ_DESTROY(hmap->vals[i]);
    hmap->keys[i] = NULL;
    hmap->vals[i] = NULL;
    hmap->len--;
    break;
  }
}

void
hmap_reset(hmap_t hmap) {
  uint32_t i;

  for(i=0; i<hmap->size; i++) {
    if(hmap->keys[i]) {
      OBJ_DESTROY(hmap->keys[i]);
    }
    if(hmap->vals[i]) {
      OBJ_DESTROY(hmap->vals[i]);
    }
  }
  memset(hmap->keys, 0, hmap->size * objp_size);
  memset(hmap->vals, 0, hmap->size * objp_size);
  hmap->len = 0;
}

void
hmap_print(obj_t obj) {
  if(obj) {
    if(obj->type == OBJ_HMAP) {
      hmap_t hmap = HMAP(obj);
      uint32_t i, first = 1;
      TR("{");
      for(i=0; i<hmap->size; i++) {
	if(hmap->keys[i] || hmap->vals[i]) {
	  if(!first)
	    TR(", ");
	  OBJ_PRINT(hmap->keys[i]);
	  TR(": ");
	  OBJ_PRINT(hmap->vals[i]);
	  first = 0;
	}
      }
      TR("}");
    }
  }
}

void
hmap_destroy(obj_t obj) {
  if(obj) {
    if(obj->type != OBJ_HMAP) {
      PWAR("destroying not hmap\n");
      return;
    }
    hmap_reset(HMAP(obj));
    mem_free(mc, obj, 0);
  }
}

void
hmap_pre_free_f(void *ptr) {
  free(HMAP(ptr)->keys);
  free(HMAP(ptr)->vals);
}
