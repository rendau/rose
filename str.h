#ifndef STR_HEADER
#define STR_HEADER

#define STR(x) ((str_t)x)

#include "obj.h"
#include <inttypes.h>

typedef struct str_st *str_t;

struct str_st {
  struct obj_st obj;
  uint32_t s;
  uint32_t l;
  char *v;
};



#define byte_2_str(x) OBJ(x)->type = OBJ_STR;
#define str_2_byte(x) OBJ(x)->type = OBJ_BYTE;

str_t
str_new();

str_t
str_new_wv(char *val, uint32_t vlen);

obj_t
str_new_wc(obj_t obj);

int
str_init(str_t str);

int
str_alloc(str_t str, uint32_t size);

int
str_expand(str_t str, uint32_t size);

void
str_cut(str_t str, uint32_t cut_size);

int
str_set_val(str_t str, char *val, uint32_t vlen);

int
str_copy(str_t dest, str_t str);

int
str_add_one(str_t str, char val);

int
str_add(str_t str, char *val, uint32_t vlen);

int
str_addf(str_t str, char *fmt, ...);

int
str_read_file(str_t str, char *fpath);

int
str_write_file(str_t str, char *fpath);

int
str_append_file(str_t str, char *fpath);

void
str_reset(str_t str);

int
str_replace(str_t str, char *s1, char *s2);

int
str_cmp(str_t str1, str_t str2);

int
str_cmp_ws(str_t str, char *s);

void
str_print(obj_t obj);

void
str_destroy(obj_t obj);

void
str_pre_free_f(void *ptr);

#endif
