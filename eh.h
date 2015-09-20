#ifndef EH_HEADER
#define EH_HEADER

#include "obj.h"

typedef struct eh_st *eh_t;

struct eh_st {
  struct obj_st obj;
  void *p1;
  void *p2;
  void *p3;
  void *h;
};

eh_t
eh_new(void *h);

void
eh_destroy(obj_t o);

#endif
