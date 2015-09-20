#include "int.h"
#include "mem.h"
#include "trace.h"
#include <stdlib.h>

#define INT_CACHE_SIZE 60

static uint16_t mc = 0;

int_t
int_new(int64_t iv) {
  int_t res;

  if(!mc) {
    mc = mem_new_ot("obj_int",
		    sizeof(struct int_st),
		    INT_CACHE_SIZE, NULL);
    ASSERT(!mc, "mem_new_ot");
  }

  res = mem_alloc(mc);
  ASSERT(!res, "mem_alloc");

  res->obj.type = OBJ_INT;
  res->obj.copy_fun = int_new_wc;
  res->obj.print_fun = int_print;
  res->obj.destroy_fun = int_destroy;
  res->v = iv;

  return res;
 error:
  return NULL;
}

obj_t
int_new_wc(obj_t obj) {
  ASSERT(!obj || (obj->type != OBJ_INT), "bad object");
  return OBJ(int_new(INT(obj)->v));
 error:
  return NULL;
}

void
int_print(obj_t obj) {
  if(obj) {
    if(obj->type == OBJ_INT) {
      TR("%jd", INT(obj)->v);
    }
  }
}

void
int_destroy(obj_t obj) {
  if(obj) {
    if(obj->type != OBJ_INT) {
      PWAR("destroying not int\n");
      return;
    }
    mem_free(mc, obj, 0);
  }
}
