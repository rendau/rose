#include "poll.h"
#include "mem.h"
#include "objtypes.h"
#include "trace.h"
#include <stdlib.h>
#include <unistd.h>

#define POLL_ITERPS_SIZE 50
#define POLL_SHOT_EVENTS_COUNT 1
#define POLL_FD_CACHE_SIZE 30

static uint8_t inited = 0;
static int epfd = -1;
struct epoll_event events[POLL_SHOT_EVENTS_COUNT];
static poll_iterp_t iterps[POLL_ITERPS_SIZE];
static uint16_t iterps_len = 0;
static uint16_t fd_mc = 0;
static time_t now;

int
poll_init() {
  if(!inited) {
    epfd = epoll_create(1);
    ASSERT(epfd==-1, "epoll_create");

    fd_mc = mem_new_ot("poll_fd",
		       sizeof(struct poll_fd_st),
		       POLL_FD_CACHE_SIZE, NULL);
    ASSERT(!fd_mc, "mem_new_ot");

    inited = 1;
  }

  return 0;
 error:
  return -1;
}

poll_fd_t
poll_add_fd(int fd, void *ro, poll_handler_t rh,
            poll_handler_t wh, poll_handler_t eh) {
  poll_fd_t poll_fd;
  int ret;

  poll_fd = poll_fd_new();
  ASSERT(!poll_fd, "poll_fd_new");

  poll_fd->fd = fd;
  poll_fd->ro = ro;
  poll_fd->rh = rh;
  poll_fd->wh = wh;
  poll_fd->eh = eh;

  ret = poll_enable_fd(poll_fd);
  ASSERT(ret, "poll_enable_fd");

  return poll_fd;
 error:
  return NULL;
}

int
poll_enable_fd(poll_fd_t poll_fd) {
  int ret;

  if(!poll_fd->enabled) {
    poll_fd->event.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
    poll_fd->event.data.ptr = poll_fd;
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, poll_fd->fd, &(poll_fd->event));
    ASSERT(ret<0, "epoll_ctl");
  }

  poll_fd->enabled = 1;

  return 0;
 error:
  return -1;
}

int
poll_mod_fd(poll_fd_t poll_fd, int write) {
  int ret;

  if(write) {
    poll_fd->event.events &= ~EPOLLIN;
    poll_fd->event.events |= EPOLLOUT;
  } else {
    poll_fd->event.events &= ~EPOLLOUT;
    poll_fd->event.events |= EPOLLIN;
  }

  ret = epoll_ctl(epfd, EPOLL_CTL_MOD, poll_fd->fd, &(poll_fd->event));
  ASSERT(ret<0, "epoll_ctl(MOD)");

  return 0;
 error:
  return -1;
}

int
poll_remove_fd(poll_fd_t poll_fd) {
  int ret;

  ret = poll_disable_fd(poll_fd);
  ASSERT(ret, "poll_disable_fd");

  poll_fd_destroy(poll_fd);

  return 0;
 error:
  return -1;
}

int
poll_disable_fd(poll_fd_t poll_fd) {
  int ret;

  if(poll_fd->enabled) {
    poll_fd->event.events = 0;
    ret = epoll_ctl(epfd, EPOLL_CTL_DEL, poll_fd->fd, &(poll_fd->event));
    if(ret)
      ASSERT(errno!=EBADF, "epoll_ctl()");
  }

  poll_fd->enabled = 0;

  return 0;
 error:
  return -1;
}

void
poll_add_iterp(poll_iterp_t iterp) {
  if(iterps_len == POLL_ITERPS_SIZE - 1) {
    PERR("iterps-massive is full");
    return;
  }
  iterps[iterps_len++] = iterp;
}

void
poll_remove_iterp(poll_iterp_t iterp) {
  uint16_t i, j;

  for(i=0; i<iterps_len; i++) {
    if(iterps[i] == iterp)
      break;
  }
  if(i < iterps_len) {
    if(i < iterps_len - 1) {
      for(j = i+1; j<iterps_len; j++)
        iterps[j-1] = iterps[j];
    }
    iterps_len--;
  }
}

int
poll_run(int freq) {
  poll_fd_t poll_fd;
  uint16_t i;
  int j, evc, ret;

  time(&now);

  for(i=0; i<iterps_len; i++) {
    ret = iterps[i](now);
    ASSERT(ret, "iterp");
  }

  evc = epoll_wait(epfd, events, POLL_SHOT_EVENTS_COUNT, freq);
  ASSERT(evc<0, "epoll_wait");
  for(j=0; j<evc; j++) {
    poll_fd = (poll_fd_t)events[j].data.ptr;
    if(events[j].events & (EPOLLERR | EPOLLRDHUP | EPOLLHUP)) {
      ret = poll_fd->eh(poll_fd);
      ASSERT(ret, "poll_fd->eh");
    } else if(events[j].events & EPOLLIN) {
      ret = poll_fd->rh(poll_fd);
      ASSERT(ret, "poll_fd->rh");
    } else if(events[j].events & EPOLLOUT) {
      ret = poll_fd->wh(poll_fd);
      ASSERT(ret, "poll_fd->wh");
    } else {
      PWAR("undefined poll event %d\n", events[j].events);
      goto error;
    }
  }

  return 0;
 error:
  return -1;
}

poll_fd_t
poll_fd_new() {
  poll_fd_t poll_fd;

  poll_fd = mem_alloc(fd_mc);
  ASSERT(!poll_fd, "mem_alloc");

  memset(poll_fd, 0, sizeof(struct poll_fd_st));

  poll_fd->obj.type = OBJ_OBJECT;
  poll_fd->fd = -1;

  return poll_fd;
 error:
  return NULL;
}

void
poll_fd_destroy(poll_fd_t poll_fd) {
  if(poll_fd) {
    /* memset(poll_fd, 0, sizeof(struct poll_fd_st)); */
    mem_free(fd_mc, poll_fd, 0);
  }
}
