#ifndef STZ_HEADER
#define STZ_HEADER

typedef struct obj_st *obj_t;
typedef struct str_st *str_t;

int
stz_pack(str_t dest, obj_t obj);

int
stz_unpack(str_t src, obj_t *res);

int
stz__test();

#endif
