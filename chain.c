#include "chain.h"
#include "mem.h"
#include "objtypes.h"
#include "trace.h"
#include <stdlib.h>

#define CHAIN_CACHE_SIZE 60
#define CHAIN_SLOT_CACHE_SIZE 60

static uint16_t mc = 0;
static uint16_t slot_mc = 0;

chain_t
chain_new() {
  chain_t res;

  if(!mc) {
    mc = mem_new_ot("obj_chain",
		    sizeof(struct chain_st),
		    CHAIN_CACHE_SIZE, NULL);
    ASSERT(!mc, "mem_new_ot");
  }

  res = mem_alloc(mc);
  ASSERT(!res, "mem_alloc");

  res->obj.type = OBJ_CHAIN;
  res->obj.copy_fun = chain_new_wc;
  res->obj.print_fun = chain_print;
  res->obj.destroy_fun = chain_destroy;

  return res;
 error:
  return NULL;
}

obj_t
chain_new_wc(obj_t obj) {
  obj_t v;
  chain_t res;
  chain_slot_t slot;
  int ret;

  ASSERT(!obj || (obj->type != OBJ_CHAIN), "bad object");

  res = chain_new();
  ASSERT(!res, "chain_new");

  slot = CHAIN(obj)->first;
  while(slot) {
    v = NULL;
    if(slot->v) {
      v = OBJ_COPY(slot->v);
      ASSERT(!v, "OBJ_COPY");
    }
    ret = chain_append(res, v);
    ASSERT(ret, "chain_append");
    slot = slot->next;
  }

  return OBJ(res);
 error:
  return NULL;
}

int
chain_append(chain_t chain, obj_t val) {
  chain_slot_t slot;

  slot = chain_slot_new();
  ASSERT(!slot, "chain_slot_new");
  slot->v = val;
  if(chain->len > 0) {
    slot->next = NULL;
    slot->prev = chain->last;
    chain->last->next = slot;
    chain->last = slot;
  } else {
    slot->next = NULL;
    slot->prev = NULL;
    chain->first = slot;
    chain->last = slot;
  }
  chain->len++;

  return 0;
 error:
  return -1;
}

int
chain_append_bool(chain_t chain, int val) {
  bool_t v;
  int ret;

  v = bool_new(val ? BOOL_TRUE : BOOL_FALSE);
  ASSERT(!v, "bool_new");

  ret = chain_append(chain, OBJ(v));
  ASSERT(ret, "chain_append");

  return 0;
 error:
  return -1;
}

int
chain_append_int(chain_t chain, uint64_t val) {
  int_t v;
  int ret;

  v = int_new(val);
  ASSERT(!v, "int_new");

  ret = chain_append(chain, OBJ(v));
  ASSERT(ret, "chain_append");

  return 0;
 error:
  return -1;
}

int
chain_append_float(chain_t chain, double val) {
  float_t v;
  int ret;

  v = float_new(val);
  ASSERT(!v, "float_new");

  ret = chain_append(chain, OBJ(v));
  ASSERT(ret, "chain_append");

  return 0;
 error:
  return -1;
}

int
chain_append_str(chain_t chain, char *val) {
  str_t v;
  int ret;

  v = str_new_wv(val, 0);
  ASSERT(!v, "str_new_wv");

  ret = chain_append(chain, OBJ(v));
  ASSERT(ret, "chain_append");

  return 0;
 error:
  return -1;
}

obj_t
chain_remove_slot(chain_t chain, chain_slot_t slot) {
  obj_t res;

  if(slot->prev)
    slot->prev->next = slot->next;
  else
    chain->first = slot->next;
  if(slot->next)
    slot->next->prev = slot->prev;
  else
    chain->last = slot->prev;

  res = slot->v;
  chain_slot_destroy(slot);
  chain->len--;

  return res;
}

void
chain_reset(chain_t chain) {
  chain_slot_t slot;

  while(chain->first) {
    slot = chain->first;
    chain->first = slot->next;
    OBJ_DESTROY(slot->v);
    chain_slot_destroy(slot);
  }
  chain->first = NULL;
  chain->last = NULL;
  chain->len = 0;
}

void
chain_print(obj_t obj) {
  if(obj) {
    if(obj->type == OBJ_CHAIN) {
      chain_t chain = CHAIN(obj);
      chain_slot_t slot;

      TR("[");
      slot = chain->first;
      while(slot) {
	if(slot != chain->first)
	  TR(", ");
	OBJ_PRINT(slot->v);
	slot = slot->next;
      }
      TR("]");
    }
  }
}

void
chain_destroy(obj_t obj) {
  if(obj) {
    if(obj->type != OBJ_CHAIN) {
      PWAR("destroying not chain\n");
      return;
    }
    chain_reset(CHAIN(obj));
    mem_free(mc, obj, 0);
  }
}

chain_slot_t
chain_slot_new() {
  chain_slot_t res;

  if(!slot_mc) {
    slot_mc = mem_new_ot("chain_slot",
			 sizeof(struct chain_slot_st),
			 CHAIN_SLOT_CACHE_SIZE, NULL);
    ASSERT(!slot_mc, "mem_new_ot");
  }

  res = mem_alloc(slot_mc);
  ASSERT(!res, "mem_alloc");

  return res;
 error:
  return NULL;
}

void
chain_slot_reset(chain_slot_t slot) {
  memset(slot, 0, sizeof(struct chain_slot_st));
}

void
chain_slot_destroy(chain_slot_t slot) {
  if(slot) {
    chain_slot_reset(slot);
    mem_free(slot_mc, slot, 0);
  }
}
