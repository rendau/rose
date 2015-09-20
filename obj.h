#ifndef OBJ_HEADER
#define OBJ_HEADER

#define OBJ(x) ((obj_t)x)
#define OBJ_COPY(x) OBJ(x)->copy_fun(OBJ(x))
#define OBJ_PRINT(x) if(x){OBJ(x)->print_fun(OBJ(x));} else {TR("(NULL)");}
#define OBJ_PRINTLN(x) do{OBJ_PRINT(x);TR("\n");}while(0)
#define OBJ_DESTROY(x) if(x){OBJ(x)->destroy_fun(OBJ(x)); (x)=NULL;}

typedef struct obj_st *obj_t;

typedef obj_t (*obj_fun_type1)(obj_t o);
typedef void (*obj_fun_type2)(obj_t o);

enum obj_types {
  OBJ_OBJECT = 0,
  OBJ_BOOL,
  OBJ_INT,
  OBJ_FLOAT,
  OBJ_STR,
  OBJ_BYTE,
  OBJ_CHAIN,
  OBJ_HMAP
};

struct obj_st {
  enum obj_types type;
  obj_fun_type1 copy_fun;
  obj_fun_type2 print_fun;
  obj_fun_type2 destroy_fun;
};

#endif
