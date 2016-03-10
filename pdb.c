#include "pdb.h"
#include "mem.h"
#include "thm.h"
#include "trace.h"
#include "objtypes.h"
#include <libpq-fe.h>

typedef struct ses_st *ses_t;
typedef struct req_st *req_t;

struct ses_st {
  struct obj_st obj;
  PGconn *c;
  req_t req;
};

struct req_st {
  struct obj_st obj;
  void *ro;
  str_t q;
  pdb_res_t r;
  pdb_arft_t cb;
};

pdb_con_t pdb_con1 = NULL;
pdb_con_t pdb_con2 = NULL;
pdb_con_t pdb_con3 = NULL;
pdb_con_t pdb_con4 = NULL;
pdb_con_t pdb_con5 = NULL;
pdb_con_t pdb_con6 = NULL;
pdb_con_t pdb_con7 = NULL;

static char inited = 0;
static uint16_t con_mc = 0;
static uint16_t ses_mc = 0;
static uint16_t res_mc = 0;
static uint16_t req_mc = 0;
static chain_t _rq;

static int _qe(PGconn *c, char *query, pdb_res_t res);
static int _qea(void *arg, void *ro, void **result);
static int _qear(void *ro, void *arg, void *result);
static pdb_con_t pdb_con_new();
static ses_t ses_new();
static pdb_res_t pdb_res_new();
static req_t req_new();
static void req_destroy(req_t req);

int
pdb_init() {
  if(!inited) {
    con_mc = mem_new_ot("pdb_con",
                        sizeof(struct pdb_con_st),
                        10, NULL);
    ASSERT(!con_mc, "mem_new_ot");

    ses_mc = mem_new_ot("pdb_ses",
                        sizeof(struct ses_st),
                        10, NULL);
    ASSERT(!ses_mc, "mem_new_ot");

    res_mc = mem_new_ot("pdb_res",
                        sizeof(struct pdb_res_st),
                        30, NULL);
    ASSERT(!res_mc, "mem_new_ot");

    req_mc = mem_new_ot("pdb_req",
                        sizeof(struct req_st),
                        30, NULL);
    ASSERT(!req_mc, "mem_new_ot");

    _rq = chain_new();
    ASSERT(!_rq, "chain_new");

    inited = 1;
  }

  return 0;
 error:
  return -1;
}

int
pdb_connect(pdb_con_t *con, char *host, char *port,
            char *db, char *user, char *psw, unsigned char cnt) {
  ses_t ses;
  int ret;

  ASSERT(!inited, "not inited");
  ASSERT(*con, "bad con");

  *con = pdb_con_new();
  ASSERT(!*con, "pdb_con_new");

  (*con)->sc = PQsetdbLogin(host, port, NULL, NULL, db, user, psw);

  while(cnt > 0) {
    ses = ses_new();
    ASSERT(!ses, "ses_new");

    ses->c = PQsetdbLogin(host, port, NULL, NULL, db, user, psw);

    ses->req = NULL;

    ret = chain_append((*con)->cl, OBJ(ses));
    ASSERT(ret, "chain_append");

    cnt--;
  }

  return 0;
 error:
  return -1;
}

pdb_res_t
pdb_qe(pdb_con_t con, char *query) {
  pdb_res_t res;
  int ret;

  ASSERT(!con, "bad con");

  res = pdb_res_new();
  ASSERT(!res, "pdb_res_new");

  ret = _qe((PGconn *)con->sc, query, res);
  ASSERT(ret, "_qe");

  return res;
 error:
  return NULL;
}

static int
_qe(PGconn *c, char *query, pdb_res_t res) {
  unsigned char st;

  res->r = PQexec(c, query);
  st = PQresultStatus((PGresult *)res->r);
  if((st == PGRES_TUPLES_OK) || (st == PGRES_COMMAND_OK)) {
    res->st = PDB_REST_OK;
    if(st == PGRES_TUPLES_OK)
      res->rc = PQntuples((PGresult *)res->r);
  } else {
    if(PQstatus(c) == CONNECTION_BAD) {
      PQreset(c);
      if(PQstatus(c) == CONNECTION_OK) {
	PQclear((PGresult *)res->r);
	res->r = PQexec(c, query);
        st = PQresultStatus((PGresult *)res->r);
        if((st == PGRES_TUPLES_OK) || (st == PGRES_COMMAND_OK)) {
          res->st = PDB_REST_OK;
          if(st == PGRES_TUPLES_OK)
            res->rc = PQntuples((PGresult *)res->r);
        } else {
          res->st = PDB_REST_ERROR;
        }
      } else {
        res->st = PDB_REST_DBNA;
      }
    } else {
      res->st = PDB_REST_ERROR;
    }
  }

  return 0;
}


int
pdb_qea(pdb_con_t con, char *query, void *ro, pdb_arft_t cb) {
  ses_t ses;
  req_t req;
  chain_slot_t chs;
  int ret;

  ASSERT(!con, "bad con");

  req = req_new();
  ASSERT(!req, "req_new");

  ret = str_set_val(req->q, query, 0);
  ASSERT(ret, "str_set_val");

  req->ro = ro;
  req->cb = cb;

  ses = NULL;
  chs = con->cl->first;
  while(chs) {
    ses = (ses_t)chs->v;
    if(!ses->req)
      break;
    ses = NULL;
    chs = chs->next;
  }

  if(ses) {
    ses->req = req;
    ret = thm_addJob(NULL, _qea, ses, _qear);
    ASSERT(ret, "thm_addJob");
  } else {
    ret = chain_append(_rq, OBJ(req));
    ASSERT(ret, "chain_append");
  }

  return 0;
 error:
  return -1;
}

static int
_qea(void *arg, void *ro, void **result) {
  ses_t ses;
  int ret;

  ses = (ses_t)arg;

  ret = _qe(ses->c, ses->req->q->v, ses->req->r);
  ASSERT(ret, "_qe");

  return 0;
 error:
  return -1;
}

static int
_qear(void *ro, void *arg, void *result) {
  req_t req;
  ses_t ses;
  int ret;

  ses = (ses_t)arg;

  req = ses->req;

  ses->req = NULL;
  if(_rq->first) {
    ses->req = (req_t)chain_remove_slot(_rq, _rq->first);
    ret = thm_addJob(NULL, _qea, ses, _qear);
    ASSERT(ret, "thm_addJob");
  }

  ret = req->cb(req->ro, req->r);
  ASSERT(ret, "cb()");

  req_destroy(req);

  return 0;
 error:
  return -1;
}

char *
pdb_gv(pdb_res_t res, uint32_t row, uint32_t col) {
  return PQgetvalue((PGresult *)res->r, row, col);
}

char *
pdb_escl(pdb_con_t con, const char *str, uint32_t len) {
  ASSERT(!inited, "not inited");
  ASSERT(!con, "bad con");

  return PQescapeLiteral(con->sc, str, len);
 error:
  return NULL;
}

void
pdb_escf(char *str) {
  PQfreemem(str);
}

static pdb_con_t
pdb_con_new() {
  pdb_con_t con;

  ASSERT(!inited, "not inited");

  con = mem_alloc(con_mc);
  ASSERT(!con, "mem_alloc");

  memset(con, 0, sizeof(struct pdb_con_st));

  con->cl = chain_new();
  ASSERT(!con->cl, "chain_new");

  return con;
 error:
  return NULL;
}

static ses_t
ses_new() {
  ses_t ses;

  ASSERT(!inited, "not inited");

  ses = mem_alloc(ses_mc);
  ASSERT(!ses, "mem_alloc");

  memset(ses, 0, sizeof(struct ses_st));

  return ses;
 error:
  return NULL;
}

static pdb_res_t
pdb_res_new() {
  pdb_res_t res;

  ASSERT(!inited, "not inited");

  res = mem_alloc(res_mc);
  ASSERT(!res, "mem_alloc");

  memset(res, 0, sizeof(struct pdb_res_st));

  return res;
 error:
  return NULL;
}

void
pdb_res_destroy(pdb_res_t res) {
  if(res) {
    if(res->r) {
      PQclear((PGresult *)res->r);
      res->r = NULL;
    }
    mem_free(res_mc, res, 0);
  }
}

static req_t
req_new() {
  req_t req;

  ASSERT(!inited, "not inited");

  req = mem_alloc(req_mc);
  ASSERT(!req, "mem_alloc");

  memset(req, 0, sizeof(struct req_st));

  req->q = str_new();
  ASSERT(!req->q, "str_new");

  req->r = pdb_res_new();
  ASSERT(!req->r, "pdb_res_new");

  return req;
 error:
  return NULL;
}

static void
req_destroy(req_t req) {
  if(req) {
    OBJ_DESTROY(req->q);
    req->q = NULL;
    pdb_res_destroy(req->r);
    req->r = NULL;
    mem_free(req_mc, req, 0);
  }
}
