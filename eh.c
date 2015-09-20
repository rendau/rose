#include "eh.h"
#include "mem.h"
#include "objtypes.h"
#include "trace.h"

#define EH_CACHE_SIZE 60

static uint16_t mc = 0;

eh_t
eh_new(void *h) {
  eh_t res;

  if(!mc) {
    mc = mem_new_ot("event_handler",
		    sizeof(struct eh_st),
		    EH_CACHE_SIZE, NULL);
    ASSERT(!mc, "mem_new_ot");
  }

  res = mem_alloc(mc);
  ASSERT(!res, "mem_alloc");

  res->obj.type = OBJ_OBJECT;
  res->obj.destroy_fun = eh_destroy;
  res->p1 = NULL;
  res->p2 = NULL;
  res->p3 = NULL;
  res->h = h;

  return res;
 error:
  return NULL;
}

void
eh_destroy(obj_t o) {
  if(o) {
    mem_free(mc, o, 0);
  }
}
