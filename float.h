#ifndef FLOAT_HEADER
#define FLOAT_HEADER

#define FLOAT(x) ((float_t)x)

#include "obj.h"

typedef struct float_st *float_t;

struct float_st {
  struct obj_st obj;
  double v;
};



float_t
float_new(double iv);

obj_t
float_new_wc(obj_t obj);

void
float_print(obj_t obj);

void
float_destroy(obj_t obj);

#endif
