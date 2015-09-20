#include "stz.h"
#include "mem.h"
#include "objtypes.h"
#include "trace.h"
#include "ns.h"
#include <stdlib.h>

static unsigned char *ind, *l_ind;
static uint16_t u16v;
static uint32_t u32v;
static uint64_t u64v;

int
stz_pack(str_t dest, obj_t obj) {
  chain_slot_t cs;
  uint64_t i;
  double d;
  int ret;

  ret = str_alloc(dest, dest->l+12);
  ASSERT(ret, "str_alloc");

  if(!obj) {
    str_add_one(dest, 0xc0);
  } else if(obj->type == OBJ_BOOL) {
    str_add_one(dest, (BOOL(obj)->v==BOOL_TRUE) ? 0xc3 : 0xc2);
  } else if(obj->type == OBJ_INT) {
    i = INT(obj)->v;
    if(INT(obj)->v < 0) {
      i = -i;
      if(i < 33) {
        str_add_one(dest, 0x100 - i);
      } else if(i < 0x100) {
        str_add_one(dest, 0xd0);
        str_add_one(dest, i);
      } else if(i < 0x10000) {
        str_add_one(dest, 0xd1);
        u16v = ns_csu16(i);
        str_add(dest, (char *)&u16v, 2);
      } else if(i < 0x100000000) {
        str_add_one(dest, 0xd2);
        u32v = ns_csu32(i);
        str_add(dest, (char *)&u32v, 4);
      } else if(i <= 0xFFFFFFFFFFFFFFFF) {
        str_add_one(dest, 0xd3);
        u64v = ns_csu64(i);
        str_add(dest, (char *)&u64v, 8);
      }
    } else {
      if(i < 0x80) {
        str_add_one(dest, i);
      } else if(i < 0x100) {
        str_add_one(dest, 0xcc);
        str_add_one(dest, i);
      } else if(i < 0x10000) {
        str_add_one(dest, 0xcd);
        u16v = ns_csu16(i);
        str_add(dest, (char *)&u16v, 2);
      } else if(i < 0x100000000) {
        str_add_one(dest, 0xce);
        u32v = ns_csu32(i);
        str_add(dest, (char *)&u32v, 4);
      } else if(i <= 0xFFFFFFFFFFFFFFFF) {
        str_add_one(dest, 0xcf);
        u64v = ns_csu64(i);
        str_add(dest, (char *)&u64v, 8);
      }
    }
  } else if(obj->type == OBJ_FLOAT) {
    str_add_one(dest, 0xcb);
    d = ns_csd(FLOAT(obj)->v);
    str_add(dest, (char *)&d, 8);
  } else if(obj->type == OBJ_STR) {
    i = STR(obj)->l;
    if(i < 32) {
      str_add_one(dest, 0xa0 + i);
    } else if(i < 0x100) {
      str_add_one(dest, 0xd9);
      str_add_one(dest, i);
    } else if(i < 0x10000) {
      str_add_one(dest, 0xda);
      u16v = ns_csu16(i);
      str_add(dest, (char *)&u16v, 2);
    } else if(i < 0x100000000) {
      str_add_one(dest, 0xdb);
      u32v = ns_csu32(i);
      str_add(dest, (char *)&u32v, 4);
    }
    ret = str_add(dest, STR(obj)->v, STR(obj)->l);
    ASSERT(ret, "str_add");
  } else if(obj->type == OBJ_BYTE) {
    i = STR(obj)->l;
    if(i < 0x100) {
      str_add_one(dest, 0xc4);
      str_add_one(dest, i);
    } else if(i < 0x10000) {
      str_add_one(dest, 0xc5);
      u16v = ns_csu16(i);
      str_add(dest, (char *)&u16v, 2);
    } else if(i < 0x100000000) {
      str_add_one(dest, 0xc6);
      u32v = ns_csu32(i);
      str_add(dest, (char *)&u32v, 4);
    }
    ret = str_add(dest, STR(obj)->v, STR(obj)->l);
    ASSERT(ret, "str_add");
  } else if(obj->type == OBJ_CHAIN) {
    i = CHAIN(obj)->len;
    if(i < 16) {
      str_add_one(dest, 0x90 + i);
    } else if(i < 0x10000) {
      str_add_one(dest, 0xdc);
      u16v = ns_csu16(i);
      str_add(dest, (char *)&u16v, 2);
    } else if(i < 0x100000000) {
      str_add_one(dest, 0xdd);
      u32v = ns_csu32(i);
      str_add(dest, (char *)&u32v, 4);
    }
    cs = CHAIN(obj)->first;
    while(cs) {
      stz_pack(dest, cs->v);
      cs = cs->next;
    }
  } else if(obj->type == OBJ_HMAP) {
    i = HMAP(obj)->len;
    if(i < 16) {
      str_add_one(dest, 0x80 + i);
    } else if(i < 0x10000) {
      str_add_one(dest, 0xde);
      u16v = ns_csu16(i);
      str_add(dest, (char *)&u16v, 2);
    } else if(i < 0x100000000) {
      str_add_one(dest, 0xdf);
      u32v = ns_csu32(i);
      str_add(dest, (char *)&u32v, 4);
    }
    for(i=0; i<HMAP(obj)->size; i++) {
      if(HMAP(obj)->keys[i]) {
        stz_pack(dest, HMAP(obj)->keys[i]);
        stz_pack(dest, HMAP(obj)->vals[i]);
      }
    }
  }

  return 0;
 error:
  return -1;
}

static int
_unpack(obj_t *res) {
  unsigned char t;
  uint64_t l;
  obj_t tmp1, tmp2;
  int ret;

  *res = NULL;

  if(ind + 1 > l_ind) return 1;
  t = *ind;
  ind++;
  if(t > 0xdf) { // negative fixnum (-32 ~ -1)
    *res = OBJ(int_new(t - 0x100));
    ASSERT(!*res, "int_new");
    return 0;
  }
  if(t < 0xc0) {
    if(t < 0x80) { // positive fixnum (0 ~ 127)
      *res = OBJ(int_new(t));
      ASSERT(!*res, "int_new");
      return 0;
    }
    if(t < 0x90) { // fixmap
      l = t - 0x80;
      t = 0x80;
    } else if(t < 0xa0) { // fixarray
      l = t - 0x90;
      t = 0x90;
    } else { // fixstring
      l = t - 0xa0;
      t = 0xa0;
    }
  }
  if(t == 0xc0) {
    *res = NULL;
  } else if(t == 0xc2) {
    *res = OBJ(bool_new(BOOL_FALSE));
    ASSERT(!*res, "bool_new");
  } else if(t == 0xc3) {
    *res = OBJ(bool_new(BOOL_TRUE));
    ASSERT(!*res, "bool_new");
  } else if(t == 0xcc) { // uint8
    if(ind + 1 > l_ind) return 1;
    *res = OBJ(int_new(*(ind++)));
    ASSERT(!*res, "int_new");
  } else if(t == 0xcd) { // uint16
    if(ind + 2 > l_ind) return 1;
    *res = OBJ(int_new(ns_csu16(*((uint16_t *)ind))));
    ASSERT(!*res, "int_new");
    ind += 2;
  } else if(t == 0xce) { // uint32
    if(ind + 4 > l_ind) return 1;
    *res = OBJ(int_new(ns_csu32(*((uint32_t *)ind))));
    ASSERT(!*res, "int_new");
    ind += 4;
  } else if(t == 0xcf) { // uint64
    if(ind + 8 > l_ind) return 1;
    *res = OBJ(int_new(ns_csu64(*((uint64_t *)ind))));
    ASSERT(!*res, "int_new");
    ind += 8;
  } else if(t == 0xd0) { // negative int8
    if(ind + 1 > l_ind) return 1;
    *res = OBJ(int_new(-*(ind++)));
    ASSERT(!*res, "int_new");
  } else if(t == 0xd1) { // negative uint16
    if(ind + 2 > l_ind) return 1;
    *res = OBJ(int_new(-ns_csu16(*((uint16_t *)ind))));
    ASSERT(!*res, "int_new");
    ind += 2;
  } else if(t == 0xd2) { // negative uint32
    if(ind + 4 > l_ind) return 1;
    *res = OBJ(int_new(-ns_csu32(*((uint32_t *)ind))));
    ASSERT(!*res, "int_new");
    ind += 4;
  } else if(t == 0xd3) { // negative uint64
    if(ind + 8 > l_ind) return 1;
    *res = OBJ(int_new(-ns_csu64(*((uint64_t *)ind))));
    ASSERT(!*res, "int_new");
    ind += 8;
  } else if(t == 0xca) { // float
    if(ind + 4 > l_ind) return 1;
    *res = OBJ(float_new(ns_csf(*((float *)ind))));
    ASSERT(!*res, "float_new");
    ind += 4;
  } else if(t == 0xcb) { // double
    if(ind + 8 > l_ind) return 1;
    *res = OBJ(float_new(ns_csd(*((double *)ind))));
    ASSERT(!*res, "float_new");
    ind += 8;
  } else if(t == 0xa0) { // fixstring
    if(l) {
      if(ind + l > l_ind) return 1;
      *res = OBJ(str_new_wv((char *)ind, l));
    } else
      *res = OBJ(str_new());
    ASSERT(!*res, "str_new(_wv)");
    ind += l;
  } else if(t == 0xd9) { // string8
    if(ind + 1 > l_ind) return 1;
    l = *ind;
    ind += 1;
    if(ind + l > l_ind) return 1;
    *res = OBJ(str_new_wv((char *)ind, l));
    ASSERT(!*res, "str_new_wv");
    ind += l;
  } else if(t == 0xda) { // string16
    if(ind + 2 > l_ind) return 1;
    l = ns_csu16(*((uint16_t *)ind));
    ind += 2;
    if(ind + l > l_ind) return 1;
    *res = OBJ(str_new_wv((char *)ind, l));
    ASSERT(!*res, "str_new_wv");
    ind += l;
  } else if(t == 0xdb) { // string32
    if(ind + 4 > l_ind) return 1;
    l = ns_csu32(*((uint32_t *)ind));
    ind += 4;
    if(ind + l > l_ind) return 1;
    *res = OBJ(str_new_wv((char *)ind, l));
    ASSERT(!*res, "str_new_wv");
    ind += l;
  } else if(t == 0xc4) { // bin8
    if(ind + 1 > l_ind) return 1;
    l = *ind;
    ind += 1;
    if(l) {
      if(ind + l > l_ind) return 1;
      *res = OBJ(str_new_wv((char *)ind, l));
    } else
      *res = OBJ(str_new());
    ASSERT(!*res, "str_new(_wv)");
    str_2_byte(STR(*res));
    ind += l;
  } else if(t == 0xc5) { // bin16
    if(ind + 2 > l_ind) return 1;
    l = ns_csu16(*((uint16_t *)ind));
    ind += 2;
    if(ind + l > l_ind) return 1;
    *res = OBJ(str_new_wv((char *)ind, l));
    ASSERT(!*res, "str_new_wv");
    str_2_byte(STR(*res));
    ind += l;
  } else if(t == 0xc6) { // bin32
    if(ind + 4 > l_ind) return 1;
    l = ns_csu32(*((uint32_t *)ind));
    ind += 4;
    if(ind + l > l_ind) return 1;
    *res = OBJ(str_new_wv((char *)ind, l));
    ASSERT(!*res, "str_new_wv");
    str_2_byte(STR(*res));
    ind += l;
  } else if(t == 0x90) { // fixarray
    *res = OBJ(chain_new());
    ASSERT(!*res, "chain_new");
    while(l > 0) {
      ret = _unpack(&tmp1);
      if(ret) return ret;
      ret = chain_append(CHAIN(*res), tmp1);
      ASSERT(ret, "chain_append");
      l--;
    }
  } else if(t == 0xdc) { // array16
    if(ind + 2 > l_ind) return 1;
    l = ns_csu16(*((uint16_t *)ind));
    ind += 2;
    *res = OBJ(chain_new());
    ASSERT(!*res, "chain_new");
    while(l > 0) {
      ret = _unpack(&tmp1);
      if(ret) return ret;
      ret = chain_append(CHAIN(*res), tmp1);
      ASSERT(ret, "chain_append");
      l--;
    }
  } else if(t == 0xdd) { // array32
    if(ind + 4 > l_ind) return 1;
    l = ns_csu32(*((uint32_t *)ind));
    ind += 4;
    *res = OBJ(chain_new());
    ASSERT(!*res, "chain_new");
    while(l > 0) {
      ret = _unpack(&tmp1);
      if(ret) return ret;
      ret = chain_append(CHAIN(*res), tmp1);
      ASSERT(ret, "chain_append");
      l--;
    }
  } else if(t == 0x80) { // fixmap
    *res = OBJ(hmap_new());
    ASSERT(!*res, "hmap_new");
    while(l > 0) {
      ret = _unpack(&tmp1);
      if(ret) return ret;
      ret = _unpack(&tmp2);
      if(ret) { OBJ_DESTROY(tmp1); return ret; }
      if(tmp1) {
        ret = hmap_set_val(HMAP(*res), tmp1, tmp2);
        ASSERT(ret, "hmap_set_val");
      } else {
        PWAR("unhashable key");
      }
      l--;
    }
  } else if(t == 0xde) { // map16
    if(ind + 2 > l_ind) return 1;
    l = ns_csu16(*((uint16_t *)ind));
    ind += 2;
    *res = OBJ(hmap_new());
    ASSERT(!*res, "hmap_new");
    while(l > 0) {
      ret = _unpack(&tmp1);
      if(ret) return ret;
      ret = _unpack(&tmp2);
      if(ret) { OBJ_DESTROY(tmp1); return ret; }
      if(tmp1) {
        ret = hmap_set_val(HMAP(*res), tmp1, tmp2);
        ASSERT(ret, "hmap_set_val");
      } else {
        PWAR("unhashable key");
      }
      l--;
    }
  } else if(t == 0xdf) { // map32
    if(ind + 4 > l_ind) return 1;
    l = ns_csu32(*((uint32_t *)ind));
    ind += 4;
    *res = OBJ(hmap_new());
    ASSERT(!*res, "hmap_new");
    while(l > 0) {
      ret = _unpack(&tmp1);
      if(ret) return ret;
      ret = _unpack(&tmp2);
      if(ret) { OBJ_DESTROY(tmp1); return ret; }
      if(tmp1) {
        ret = hmap_set_val(HMAP(*res), tmp1, tmp2);
        ASSERT(ret, "hmap_set_val");
      } else {
        PWAR("unhashable key");
      }
      l--;
    }
  }

  return 0;
 error:
  return -1;
}

int
stz_unpack(str_t src, obj_t *res) {
  if(!src->l) {
    *res = NULL;
    return 1;
  }
  ind = (unsigned char *)src->v;
  l_ind = (unsigned char *)(src->v+src->l);
  return _unpack(res);
}

int
stz__test() {
  hmap_t src, dst;
  str_t bytes;
  int ret;

  src = hmap_new();
  ASSERT(!src, "hmap_new");

  ret = hmap_set_val_ks_vi(src, "int", -0x1000000000);
  ASSERT(ret, "hmap_set_val_ks_v");

  ret = hmap_set_val_ks_vf(src, "float", 7.1234);
  ASSERT(ret, "hmap_set_val_ks_v");

  ret = hmap_set_val_ks_vs(src, "str", "hello");
  ASSERT(ret, "hmap_set_val_ks_v");

  OBJ_PRINT(src); TR("\n");

  bytes = str_new();
  ASSERT(!bytes, "str_new");

  ret = stz_pack(bytes, OBJ(src));
  ASSERT(ret, "stz_pack");

  ret = stz_unpack(bytes, (obj_t *)&dst);
  ASSERT(ret==-1, "stz_unpack");

  OBJ_PRINT(dst); TR("\n");
  
  return 0;
 error:
  return -1;
}
