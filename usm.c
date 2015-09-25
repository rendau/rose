#include "usm.h"
#include "mem.h"
#include "objtypes.h"
#include "poll.h"
#include "trace.h"
#include "sm.h"
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define USM_SOCK_CACHE_SIZE 30

static uint8_t inited;
static uint16_t sock_mc;
static int _ss;
static struct sockaddr_in si1;
static socklen_t slen;

static int usm__read_h(poll_fd_t poll_fd);
static int usm__close_h(poll_fd_t poll_fd);
static usm_sock_t usm_sock_new();
static void usm_sock_destroy(usm_sock_t sock);

int
usm_init() {
  int ret;

  if(!inited) {
    slen = sizeof(struct sockaddr_in);

    ret = poll_init();
    ASSERT(ret, "poll_init");

    ret = sm_init();
    ASSERT(ret, "sm_init");

    sock_mc = mem_new_ot("usm_sock",
			 sizeof(struct usm_sock_st),
			 USM_SOCK_CACHE_SIZE, NULL);
    ASSERT(!sock_mc, "mem_new_ot");

    _ss = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ASSERT(_ss<0, "socket");
    sm_set_fd_nb(_ss);

    inited = 1;
  }

  return 0;
 error:
  return -1;
}

usm_sock_t
usm_listen(int port, uint32_t bs) {
  usm_sock_t sock;
  int newfd, ret, v;

  newfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  ASSERT(newfd<0, "socket");

  v = 1;
  ret = setsockopt(newfd, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v));
  ASSERT(ret<0, "setsockopt");

  memset(&si1, 0, slen);
  si1.sin_family = AF_INET;
  si1.sin_port = htons(port);
  si1.sin_addr.s_addr = INADDR_ANY;

  ret = bind(newfd, (struct sockaddr *)&si1, slen);
  ASSERT(ret<0, "bind");

  sm_set_fd_nb(newfd);

  sock = usm_sock_new();
  ASSERT(!sock, "usm_sock_new");

  ret = str_alloc(sock->rb, bs);
  ASSERT(ret, "str_alloc");

  sock->poll_fd = poll_add_fd(newfd, sock, usm__read_h, NULL, usm__close_h);
  ASSERT(!sock->poll_fd, "poll_add_fd");

  return sock;
 error:
  return NULL;
}

int
usm_send(uint32_t da, uint32_t dp, char *d, uint32_t ds) {
  int ret;

  memset(&si1, 0, slen);
  si1.sin_family = AF_INET;
  si1.sin_port = htons(dp);
  si1.sin_addr.s_addr = da;

  ret = sendto(_ss, d, ds, 0, (struct sockaddr *)&si1, slen);
  ASSERT(ret!=ds, "sendto");

  return 0;
 error:
  return -1;
}

int
usm_close(usm_sock_t sock) {
  int ret;

  close(sock->poll_fd->fd);

  ret = poll_remove_fd(sock->poll_fd);
  ASSERT(ret, "poll_remove_fd");

  usm_sock_destroy(sock);

  return 0;
 error:
  return -1;
}

static int
usm__read_h(poll_fd_t poll_fd) {
  usm_sock_t sock;
  socklen_t sl;
  int rc, ret;

  sock = (usm_sock_t)poll_fd->ro;

  memset(&si1, 0, slen);

  rc = recvfrom(poll_fd->fd, sock->rb->v, sock->rb->s, 0,
                 (struct sockaddr *)&si1, &sl);
  ASSERT(rc<=0, "recvfrom ret=%d", rc);

  ASSERT(!sock->rh, "has not rh");

  ret = sock->rh(sock, (uint32_t)si1.sin_addr.s_addr, sock->rb->v, rc);
  ASSERT(ret, "rh");

  return 0;
 error:
  return -1;
}

static int
usm__close_h(poll_fd_t poll_fd) {
  PWAR("udp socket closed\n");
  return 0;
}

static usm_sock_t
usm_sock_new() {
  usm_sock_t sock;

  sock = mem_alloc(sock_mc);
  ASSERT(!sock, "mem_alloc");

  memset(sock, 0, sizeof(struct usm_sock_st));

  sock->rb = str_new();
  ASSERT(!sock->rb, "str_new");

  return sock;
 error:
  return NULL;
}

static void
usm_sock_destroy(usm_sock_t sock) {
  if(sock) {
    OBJ_DESTROY(sock->rb);
    mem_free(sock_mc, sock, 0);
  }
}
