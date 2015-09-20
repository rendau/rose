#include "bool.h"
#include "mem.h"
#include "trace.h"
#include <stdlib.h>

#define BOOL_CACHE_SIZE 60

static uint16_t mc = 0;

bool_t
bool_new(enum bool_values iv) {
  bool_t res;

  if(!mc) {
    mc = mem_new_ot("obj_bool",
		    sizeof(struct bool_st),
		    BOOL_CACHE_SIZE, NULL);
    ASSERT(!mc, "mem_new_ot");
  }

  res = mem_alloc(mc);
  ASSERT(!res, "mem_alloc");

  res->obj.type = OBJ_BOOL;
  res->obj.copy_fun = bool_new_wc;
  res->obj.print_fun = bool_print;
  res->obj.destroy_fun = bool_destroy;
  res->v = iv;

  return res;
 error:
  return NULL;
}

obj_t
bool_new_wc(obj_t obj) {
  ASSERT(!obj || (obj->type != OBJ_BOOL), "bad object");
  return OBJ(bool_new(BOOL(obj)->v));
 error:
  return NULL;
}

void
bool_print(obj_t obj) {
  if(obj) {
    if(obj->type == OBJ_BOOL) {
      TR((BOOL(obj)->v==BOOL_TRUE)?"true":"false");
    }
  }
}

void
bool_destroy(obj_t obj) {
  if(obj) {
    if(obj->type != OBJ_BOOL) {
      PWAR("destroying not bool\n");
      return;
    }
    mem_free(mc, obj, 0);
  }
}
