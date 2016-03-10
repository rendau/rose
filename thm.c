#include "thm.h"
#include "objtypes.h"
#include "mem.h"
#include "poll.h"
#include "trace.h"
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

typedef struct tctx_st *tctx_t;
typedef struct job_st *job_t;

struct tctx_st {
  struct obj_st obj;
  pthread_t tid;
  unsigned char id;
  int p1[2]; // for thread
  int p2[2]; // for root
  job_t job;
};

struct job_st {
  struct obj_st obj;
  void *ro;
  thm_jft_t jf;
  void *ja;
  thm_rft_t rf;
  void *res;
};

static char inited;
static uint16_t tctx_mc;
static uint16_t job_mc;
static chain_t _thl;
static chain_t _jl;

static void * _thread_routine(void *arg);
static int _th_read_h(poll_fd_t poll_fd);
static int _th_write_h(poll_fd_t poll_fd);
static int _th_close_h(poll_fd_t poll_fd);
static job_t job_new();
static void job_destroy(job_t job);

int
thm_init(unsigned char len) {
  tctx_t tctx;
  poll_fd_t poll_fd;
  unsigned char i;
  int ret;

  ASSERT(inited, "twice init");
  ASSERT(!len, "bad len");

  if(!inited) {
    inited = 1;

    ret = poll_init();
    ASSERT(ret, "poll_init");

    tctx_mc = mem_new_ot("thm_tctx",
                         sizeof(struct tctx_st),
                         20, NULL);
    ASSERT(!tctx_mc, "mem_new_ot");

    job_mc = mem_new_ot("thm_job",
                         sizeof(struct job_st),
                         60, NULL);
    ASSERT(!job_mc, "mem_new_ot");

    _thl = chain_new();
    ASSERT(!_thl, "chain_new");

    _jl = chain_new();
    ASSERT(!_jl, "chain_new");

    for(i=0; i<len; i++) {
      tctx = mem_alloc(tctx_mc);
      ASSERT(!tctx, "mem_alloc");

      tctx->id = i+1;

      ret = pipe(tctx->p1);
      ASSERT(ret, "pipe");
      ret = pipe(tctx->p2);
      ASSERT(ret, "pipe");

      poll_fd = poll_add_fd(tctx->p2[0], tctx, _th_read_h, _th_write_h, _th_close_h);
      ASSERT(!poll_fd, "poll_add_fd");

      tctx->job = NULL;

      ret = pthread_create(&tctx->tid, NULL, _thread_routine, (void*)tctx);
      ASSERT(ret, "pthread_create");

      ret = chain_append(_thl, OBJ(tctx));
      ASSERT(ret, "chain_append");
    }
  }

  return 0;
 error:
  return -1;
}

char
thm_is_inited() {
  return inited;
}

unsigned char
thm_threads_cnt() {
  if(inited)
    return (unsigned char)_thl->len;
  return 0;
}

int
thm_addJob(void *ro, thm_jft_t jf, void *jarg, thm_rft_t rf) {
  tctx_t tctx;
  job_t job;
  chain_slot_t chs;
  int ret;

  job = job_new();
  ASSERT(!job, "job_new");

  job->ro = ro;
  job->jf = jf;
  job->ja = jarg;
  job->rf = rf;

  tctx = NULL;
  chs = _thl->first;
  while(chs) {
    tctx = (tctx_t)chs->v;
    if(!tctx->job)
      break;
    tctx = NULL;
    chs = chs->next;
  }

  if(tctx) {
    tctx->job = job;
    ret = write(tctx->p1[1], "a", 1);
    ASSERT(ret<=0, "write");
  } else {
    ret = chain_append(_jl, OBJ(job));
    ASSERT(ret, "chain_append");
  }

  return 0;
 error:
  return -1;
}

static void *
_thread_routine(void *arg) {
  tctx_t ctx;
  job_t job;
  char byte;
  int ret;

  ctx = (tctx_t)arg;

  while(1) {
    ret = read(ctx->p1[0], &byte, 1);
    ASSERT(ret<0, "read");
    if(ret > 0) {
      if(ctx->job) {
	job = ctx->job;
	job->res = NULL;
	ret = job->jf(job->ja, job->ro, &job->res);
	ASSERT(ret, "job_fun()");
	ret = write(ctx->p2[1], "a", 1);
	ASSERT(ret<=0, "write");
      }
    } else
      break;
  }

 error:
  close(ctx->p2[1]);
  return NULL;
}

static int
_th_read_h(poll_fd_t poll_fd) {
  tctx_t tctx;
  job_t job;
  char byte;
  int ret;

  tctx = (tctx_t)poll_fd->ro;

  ret = read(poll_fd->fd, &byte, 1);
  ASSERT(ret<0, "read");
  if(ret == 0) {
    return _th_close_h(poll_fd);
  } else if(ret > 0) {
    if(tctx->job) {
      job = tctx->job;
      tctx->job = NULL;
      if(_jl->first) {
	tctx->job = (job_t)chain_remove_slot(_jl, _jl->first);
	ret = write(tctx->p1[1], "a", 1);
	ASSERT(ret<=0, "write");
      }
      ret = job->rf(job->ro, job->ja, job->res);
      ASSERT(ret, "result_fun()");
      job_destroy(job);
    }
  }

  return 0;
 error:
  return -1;
}

static int
_th_write_h(poll_fd_t poll_fd) {
  return 0;
}

static int
_th_close_h(poll_fd_t poll_fd) {
  return 0;
}

static job_t
job_new() {
  job_t job;

  job = mem_alloc(job_mc);
  ASSERT(!job, "mem_alloc");

  memset(job, 0, sizeof(struct job_st));

  return job;
 error:
  return NULL;
}

static void
job_destroy(job_t job) {
  if(job) {
    mem_free(job_mc, job, 0);
  }
}
