#ifndef USM_HEADER
#define USM_HEADER

#include "obj.h"
#include <inttypes.h>
#include <time.h>

typedef struct usm_sock_st *usm_sock_t;

typedef struct poll_fd_st *poll_fd_t;
typedef struct str_st *str_t;

typedef int (*usm_cc_handler_t) (usm_sock_t sock);
typedef int (*usm_r_handler_t) (usm_sock_t sock, uint32_t sa, char *d, int ds);

struct usm_sock_st {
  struct obj_st obj;
  poll_fd_t poll_fd;
  str_t rb;
  void *ro;
  usm_r_handler_t rh;
  /* usm_cc_handler_t eh; */
};



int
usm_init();

usm_sock_t
usm_listen(int port, uint32_t bs);

int
usm_send(uint32_t da, uint32_t dp, char *d, uint32_t ds);

int
usm_close(usm_sock_t sock);

#endif
