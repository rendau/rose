#include "poll.h"
#include "mem.h"
#include "objtypes.h"
#include "trace.h"
#include <stdlib.h>
#include <unistd.h>

#define SHPL_SIZE 25
#define OTPS_SIZE 25
#define SHOT_EVENTS_COUNT 10
#define FD_CACHE_SIZE 50

struct shp_st {
  poll_p_t p;
  uint32_t freq;
  time_t lct;
};

static uint8_t inited = 0;
static int epfd = -1;
struct epoll_event events[SHOT_EVENTS_COUNT];
static struct shp_st shpl[SHPL_SIZE]; // sheduled procedures list
static poll_p_t otpl[OTPS_SIZE]; // one-time procedures list
static uint16_t shpl_len, fd_mc;
static time_t now;

int
poll_init() {
  if(!inited) {
    epfd = epoll_create(1);
    ASSERT(epfd==-1, "epoll_create");

    fd_mc = mem_new_ot("poll_fd",
		       sizeof(struct poll_fd_st),
		       FD_CACHE_SIZE, NULL);
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

int
poll_add_shp(poll_p_t p, uint32_t freq) {
  ASSERT(!p || !freq, "bad arguments");
  ASSERT(shpl_len==SHPL_SIZE, "shpl-massive is full");

  shpl[shpl_len].p = p;
  shpl[shpl_len].freq = freq;
  shpl[shpl_len].lct = time(NULL);

  shpl_len++;

  return 0;
 error:
  return -1;
}

int
poll_add_otp(poll_p_t p) {
  int i;

  ASSERT(!p, "bad arguments");

  for(i=0; i<OTPS_SIZE; i++) {
    if(!otpl[i])
      break;
  }
  ASSERT(i==OTPS_SIZE, "otpl-massive is full");

  otpl[i] = p;

  return 0;
 error:
  return -1;
}

int
poll_run(int freq) {
  poll_fd_t poll_fd;
  int i, j, evc, ret;

  time(&now);

  for(i=0; i<shpl_len; i++) {
    if((now - shpl[i].lct) >= shpl[i].freq) {
      ret = shpl[i].p(now);
      ASSERT(ret, "shp");
      shpl[i].lct = now;
      j = 0;
      while(otpl[j]) {
        ret = otpl[j](now);
        ASSERT(ret, "otp");
        otpl[j++] = NULL;
      }
    }
  }

  evc = epoll_wait(epfd, events, SHOT_EVENTS_COUNT, freq);
  ASSERT(evc<0, "epoll_wait");
  for(i=0; i<evc; i++) {
    poll_fd = (poll_fd_t)events[i].data.ptr;
    if(events[i].events & (EPOLLERR | EPOLLRDHUP | EPOLLHUP)) {
      ret = poll_fd->eh(poll_fd);
      ASSERT(ret, "poll_fd->eh");
    } else if(events[i].events & EPOLLIN) {
      ret = poll_fd->rh(poll_fd);
      ASSERT(ret, "poll_fd->rh");
    } else if(events[i].events & EPOLLOUT) {
      ret = poll_fd->wh(poll_fd);
      ASSERT(ret, "poll_fd->wh");
    } else {
      PWAR("undefined poll event %d\n", events[i].events);
      goto error;
    }
    j = 0;
    while(otpl[j]) {
      ret = otpl[j](now);
      ASSERT(ret, "otp");
      otpl[j++] = NULL;
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
