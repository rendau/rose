#ifndef POLL_HEADER
#define POLL_HEADER

#include "obj.h"
#include <sys/epoll.h>
#include <time.h>

typedef struct poll_fd_st *poll_fd_t;

typedef int (*poll_handler_t) (poll_fd_t poll_fd);
typedef int (*poll_iterp_t) (time_t);

struct poll_fd_st {
  struct obj_st obj;
  int fd;
  void *ro;
  poll_handler_t rh;
  poll_handler_t wh;
  poll_handler_t eh;
  struct epoll_event event;
  unsigned char enabled;
};



int
poll_init();

poll_fd_t
poll_add_fd(int fd, void *ro, poll_handler_t rh,
            poll_handler_t wh, poll_handler_t eh);

int
poll_enable_fd(poll_fd_t poll_fd);

int
poll_mod_fd(poll_fd_t poll_fd, int write);

int
poll_remove_fd(poll_fd_t poll_fd);

int
poll_disable_fd(poll_fd_t poll_fd);

void
poll_add_iterp(poll_iterp_t iterp);

void
poll_remove_iterp(poll_iterp_t iterp);

int
poll_run(int freq);

poll_fd_t
poll_fd_new();

void
poll_fd_destroy(poll_fd_t poll_fd);

#endif
