#include "float.h"
#include "mem.h"
#include "trace.h"
#include <stdlib.h>

#define FLOAT_CACHE_SIZE 60

static uint16_t mc = 0;

float_t
float_new(double iv) {
  float_t res;

  if(!mc) {
    mc = mem_new_ot("obj_float",
		    sizeof(struct float_st),
		    FLOAT_CACHE_SIZE, NULL);
    ASSERT(!mc, "mem_new_ot");
  }

  res = mem_alloc(mc);
  ASSERT(!res, "mem_alloc");

  res->obj.type = OBJ_FLOAT;
  res->obj.copy_fun = float_new_wc;
  res->obj.print_fun = float_print;
  res->obj.destroy_fun = float_destroy;
  res->v = iv;

  return res;
 error:
  return NULL;
}

obj_t
float_new_wc(obj_t obj) {
  ASSERT(!obj || (obj->type != OBJ_FLOAT), "bad object");
  return OBJ(float_new(FLOAT(obj)->v));
 error:
  return NULL;
}

void
float_print(obj_t obj) {
  if(obj) {
    if(obj->type == OBJ_FLOAT) {
      TR("%f", FLOAT(obj)->v);
    }
  }
}

void
float_destroy(obj_t obj) {
  if(obj) {
    if(obj->type != OBJ_FLOAT) {
      PWAR("destroying not float\n");
      return;
    }
    mem_free(mc, obj, 0);
  }
}
