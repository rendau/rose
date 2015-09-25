#include "str.h"
#include "mem.h"
#include "trace.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define STR_INIT_SIZE 60
#define STR_CACHE_SIZE 100
#define STR_MEM_CRITICAL 4096

static uint16_t mc = 0;

str_t
str_new() {
  str_t res;
  int ret;

  if(!mc) {
    mc = mem_new_ot("obj_str",
		    sizeof(struct str_st),
		    STR_CACHE_SIZE, str_pre_free_f);
    ASSERT(!mc, "mem_new_ot");
  }

  res = mem_alloc(mc);
  ASSERT(!res, "mem_alloc");

  if(res->s) {
    str_reset(res);
  } else {
    ret = str_init(res);
    ASSERT(ret, "str_init");
  }

  res->obj.type = OBJ_STR;
  res->obj.copy_fun = str_new_wc;
  res->obj.print_fun = str_print;
  res->obj.destroy_fun = str_destroy;

  return res;
 error:
  return NULL;
}

str_t
str_new_wv(char *val, uint32_t vlen) {
  str_t res;
  int ret;

  res = str_new();
  ASSERT(!res, "str_new");
  ret = str_set_val(res, val, vlen);
  ASSERT(ret, "str_set_val");

  return res;
 error:
  return NULL;
}

obj_t
str_new_wc(obj_t obj) {
  str_t res;

  ASSERT(!obj || ((obj->type != OBJ_STR) &&
                  (obj->type != OBJ_BYTE)), "bad object");

  res = str_new_wv(STR(obj)->v, STR(obj)->l);
  ASSERT(!res, "str_new_wv");

  res->obj.type = obj->type;

  return OBJ(res);
 error:
  return NULL;
}

int
str_init(str_t str) {
  ASSERT(str->s, "twice init");
  str->v = malloc(STR_INIT_SIZE);
  ASSERT(!str->v, "malloc");
  str->s = STR_INIT_SIZE;
  str_reset(str);

  return 0;
 error:
  return -1;
}

int
str_alloc(str_t str, uint32_t size) {
  void *ptr;

  ASSERT(!str->s, "not inited");

  if(str->s < size) {
    while(str->s < size)
      str->s += 100;
    ptr = realloc(str->v, str->s);
    ASSERT(!ptr, "realloc");
    str->v = ptr;
  }

  return 0;
 error:
  return -1;
}

int
str_expand(str_t str, uint32_t size) {
  ASSERT(str_alloc(str, size+1), "str_alloc");
  str->l = size;
  str->v[str->l] = 0;

  return 0;
 error:
  return -1;
}

void
str_cut(str_t str, uint32_t cut_size) {
  if(cut_size >= str->s)
    cut_size = str->s - 1;
  str->l = cut_size;
  str->v[str->l] = 0;
}

int
str_set_val(str_t str, char *val, uint32_t vlen) {
  uint32_t l;

  str_reset(str);
  l = vlen ? vlen : strlen(val);
  if(l) {
    ASSERT(str_expand(str, l), "str_expand");
    memcpy(str->v, val, str->l);
  }

  return 0;
 error:
  return -1;
}

int
str_copy(str_t dest, str_t str) {
  return str_set_val(dest, str->v, str->l);
}

int
str_add_one(str_t str, char val) {
  ASSERT(str_expand(str, str->l+1), "str_alloc");
  str->v[str->l-1] = val;

  return 0;
 error:
  return -1;
}

int
str_add(str_t str, char *val, uint32_t vlen) {
  int l;

  l = vlen ? vlen : strlen(val);
  if(l) {
    ASSERT(str_expand(str, str->l+l), "str_expand");
    memcpy(str->v+str->l-l, val, l);
  }

  return 0;
 error:
  return -1;
}

int
str_addf(str_t str, char *fmt, ...) {
  va_list args;
  uint32_t l;

  va_start(args, fmt);
  l = vsnprintf(NULL, 0, fmt, args);
  va_end(args);
  ASSERT(l<0, "vsnprintf");
  ASSERT(str_expand(str, str->l+l), "str_expand");
  va_start(args, fmt);
  vsnprintf(str->v+str->l-l, l+1, fmt, args);
  va_end(args);

  return 0;
 error:
  return -1;
}

int
str_read_file(str_t str, char *fpath) {
  FILE *file;
  int cnt, ret;

  file = fopen(fpath, "r");
  if(!file)
    return 1;

  ret = str_alloc(str, str->l+101);
  ASSERT(ret, "str_alloc");
  while((cnt = fread(str->v+str->l, 1, 100, file)) > 0) {
    ret = str_expand(str, str->l+cnt);
    ASSERT(ret, "str_expand");
    ret = str_alloc(str, str->l+101);
    ASSERT(ret, "str_alloc");
  }
  ASSERT(!feof(file), "fread");
  fclose(file);

  return 0;
 error:
  return -1;
}

int
str_write_file(str_t str, char *fpath) {
  FILE *file;
  int wc;

  file = fopen(fpath, "w");
  if(!file)
    return 1;

  wc = fwrite(str->v, 1, str->l, file);
  ASSERT(wc!=str->l, "fwrite");

  fclose(file);

  return 0;
 error:
  return -1;
}

void
str_reset(str_t str) {
  str->l = 0;
  str->v[0] = 0;
}

int
str_replace(str_t str, char *s1, char *s2) {
  str_t buf;
  char *ptr;
  int ret, s1_l, s2_l;

  s1_l = strlen(s1);
  s2_l = strlen(s2);

  ptr = strstr(str->v, s1);
  while(ptr) {
    buf = str_new_wv(ptr+s1_l, str->l-((ptr+s1_l)-str->v));
    ASSERT(!buf, "str_new_wv");
    str_cut(str, ptr-str->v);
    ret = str_add(str, s2, s2_l);
    ASSERT(ret, "str_add");
    ret = str_add(str, buf->v, buf->l);
    ASSERT(ret, "str_add");
    OBJ_DESTROY(buf);
    ptr = strstr(str->v, s1);
  }

  return 0;
 error:
  return -1;
}

int
str_cmp(str_t str1, str_t str2) {
  return strcmp(str1->v, str2->v);
}

int
str_cmp_ws(str_t str, char *s) {
  return strcmp(str->v, s);
}

void
str_print(obj_t obj) {
  if(obj) {
    if(obj->type == OBJ_STR) {
      if(STR(obj)->l < 100)
	TR("'%s'", STR(obj)->v);
      else
	TR("'%.*s...'", 100, STR(obj)->v);
    } else if(obj->type == OBJ_BYTE) {
      uint32_t i;
      str_t str = STR(obj);

      if(str->l < 100) {
	TR("'");
	for(i=0; i<str->l; i++) {
	  if(i == 0)
	    TR("%02x", (unsigned char)str->v[i]);
	  else
	    TR(" %02x", (unsigned char)str->v[i]);
	}
	TR("'");
      } else {
	TR("bytes(%d)", str->l);
      }
    }
  }
}

void
str_destroy(obj_t obj) {
  if(obj) {
    if((obj->type != OBJ_STR) &&
       (obj->type != OBJ_BYTE)) {
      PWAR("destroying not str or byte\n");
      return;
    }
    if(!STR(obj)->s) {
      PWAR("destroying not inited str or byte\n");
    }
    if(STR(obj)->s < STR_MEM_CRITICAL) {
      str_reset(STR(obj));
      mem_free(mc, obj, 0);
    } else
      mem_free(mc, obj, 1);
  }
}

void
str_pre_free_f(void *ptr) {
  str_t str;

  str = STR(ptr);

  if(str->s) {
    str->v[0] = 0;
    free(str->v);
  }

  str->v = NULL;
  str->s = 0;
  str->l = 0;
}
