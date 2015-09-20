#ifndef BOOL_HEADER
#define BOOL_HEADER

#define BOOL(x) ((bool_t)x)

#include "obj.h"

typedef struct bool_st *bool_t;

enum bool_values {
  BOOL_FALSE,
  BOOL_TRUE,
};

struct bool_st {
  struct obj_st obj;
  enum bool_values v;
};



bool_t
bool_new(enum bool_values iv);

obj_t
bool_new_wc(obj_t obj);

void
bool_print(obj_t obj);

void
bool_destroy(obj_t obj);

#endif
