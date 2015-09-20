#ifndef PDB_HEADER
#define PDB_HEADER

#include "obj.h"
#include <inttypes.h>

typedef struct pdb_con_st *pdb_con_t;
typedef struct pdb_res_st *pdb_res_t;

typedef struct chain_st *chain_t;

typedef int (*pdb_arft_t) (void *ro, pdb_res_t res);

enum pdb_res_status_type { 
  PDB_REST_OK = 0,
  PDB_REST_DBNA,
  PDB_REST_ERROR
};

struct pdb_con_st {
  struct obj_st obj;
  void *sc;
  chain_t cl;
};

struct pdb_res_st {
  struct obj_st obj;
  void *r;
  enum pdb_res_status_type st;
  uint32_t rc;
};

extern pdb_con_t pdb_con1;
extern pdb_con_t pdb_con2;
extern pdb_con_t pdb_con3;
extern pdb_con_t pdb_con4;
extern pdb_con_t pdb_con5;
extern pdb_con_t pdb_con6;
extern pdb_con_t pdb_con7;

int
pdb_init();

int
pdb_connect(pdb_con_t *con, char *host, char *port,
            char *db, char *user, char *psw, unsigned char cnt);

pdb_res_t
pdb_qe(pdb_con_t con, char *query);

int
pdb_qea(pdb_con_t con, char *query, void *ro, pdb_arft_t cb);

char *
pdb_gv(pdb_res_t res, uint32_t row, uint32_t col);

char *
pdb_escl(pdb_con_t con, const char *str, uint32_t len);

void
pdb_escf(char *str);

void
pdb_res_destroy(pdb_res_t res);

#endif
