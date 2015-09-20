#ifndef CHAIN_HEADER
#define CHAIN_HEADER

#define CHAIN(x) ((chain_t)x)

#include "obj.h"
#include "inttypes.h"

typedef struct chain_st *chain_t;
typedef struct chain_slot_st *chain_slot_t;

struct chain_st {
  struct obj_st obj;
  chain_slot_t first;
  chain_slot_t last;
  uint32_t len;
};

struct chain_slot_st {
  obj_t v;
  chain_slot_t prev;
  chain_slot_t next;
};



chain_t
chain_new();

obj_t
chain_new_wc(obj_t obj);

int
chain_append(chain_t chain, obj_t val);

int
chain_append_bool(chain_t chain, int val);

int
chain_append_int(chain_t chain, uint64_t val);

int
chain_append_float(chain_t chain, double val);

int
chain_append_str(chain_t chain, char *val);

obj_t
chain_remove_slot(chain_t chain, chain_slot_t slot);

void
chain_reset(chain_t chain);

void
chain_print(obj_t obj);

void
chain_destroy(obj_t obj);

chain_slot_t
chain_slot_new();

void
chain_slot_reset(chain_slot_t slot);

void
chain_slot_destroy(chain_slot_t slot);

#endif
