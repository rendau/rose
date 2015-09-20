#ifndef INT_HEADER
#define INT_HEADER

#define INT(x) ((int_t)x)

#include "obj.h"
#include <inttypes.h>

typedef struct int_st *int_t;

struct int_st {
  struct obj_st obj;
  int64_t v;
};



int_t
int_new(int64_t iv);

obj_t
int_new_wc(obj_t obj);

void
int_print(obj_t obj);

void
int_destroy(obj_t obj);

#endif
