#ifndef HMAP_HEADER
#define HMAP_HEADER

#define HMAP(x) ((hmap_t)x)

#include "obj.h"
#include "inttypes.h"

typedef struct hmap_st *hmap_t;

typedef struct str_st *str_t;

struct hmap_st {
  struct obj_st obj;
  obj_t *keys;
  obj_t *vals;
  uint32_t size;
  uint32_t len;
};



hmap_t
hmap_new();

obj_t
hmap_new_wc(obj_t obj);

int
hmap_expand(hmap_t hmap);

int
hmap_set_val(hmap_t hmap, obj_t k, obj_t v);

int
hmap_set_val_ks(hmap_t hmap, char *k, obj_t v);

int
hmap_set_val_ks_vs(hmap_t hmap, char *k, char *v);

int
hmap_set_val_ks_vsc(hmap_t hmap, char *k, str_t v);

int
hmap_set_val_ks_vb(hmap_t hmap, char *k, int v);

int
hmap_set_val_ks_vi(hmap_t hmap, char *k, int64_t v);

int
hmap_set_val_ks_vf(hmap_t hmap, char *k, double v);

obj_t
hmap_get_val_ks(hmap_t hmap, char *k, int rk);

void
hmap_remove_key_ks(hmap_t hmap, char *k);

void
hmap_reset(hmap_t hmap);

void
hmap_print(obj_t obj);

void
hmap_destroy(obj_t obj);

void
hmap_pre_free_f(void *ptr);

#endif
