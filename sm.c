#include "sm.h"
#include "mem.h"
#include "objtypes.h"
#include "poll.h"
#include "trace.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <fcntl.h>
#include <openssl/err.h>

#define SM_SOCK_CACHE_SIZE 30

#define KA_IDLE 20
#define KA_INT 5
#define KA_CNT 2

static uint8_t inited = 0;
static uint8_t ssl_inited = 0;
static chain_t wsocks, tsocks, csocks;
static uint16_t sock_mc = 0;
static unsigned char _mac[SM_MAC_LEN];

static void sm_ssl_init();
static int sm_iterp(time_t now);
static int sm_set_fd_ka(int fd);
static int sm__accept_h(poll_fd_t poll_fd);
static int sm__sslAccept_h(sm_sock_t sock);
static int sm__connect_h(poll_fd_t poll_fd, int stop);
static int sm__sslConnect_h(sm_sock_t sock);
static int sm__read_h(poll_fd_t poll_fd);
static int sm__sslRead_h(sm_sock_t sock);
static int sm__write_h(poll_fd_t poll_fd);
static int sm__sslWrite_h(sm_sock_t sock);
static int sm__close_h(poll_fd_t poll_fd);
static sm_sock_t sm_sock_new(sm_sock_type_e type);
static void sm_sock_destroy(sm_sock_t sock);

int
sm_init() {
  int ret;

  if(!inited) {
    ret = poll_init();
    ASSERT(ret, "poll_init");

    wsocks = chain_new();
    ASSERT(!wsocks, "chain_new");

    tsocks = chain_new();
    ASSERT(!tsocks, "chain_new");

    csocks = chain_new();
    ASSERT(!csocks, "chain_new");

    sock_mc = mem_new_ot("sm_sock",
			 sizeof(struct sm_sock_st),
			 SM_SOCK_CACHE_SIZE, NULL);
    ASSERT(!sock_mc, "mem_new_ot");

    poll_add_iterp(sm_iterp);

    inited = 1;
  }

  return 0;
 error:
  return -1;
}

static void
sm_ssl_init() {
  if(!ssl_inited) {
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    ssl_inited = 1;
  }
}

static int
sm_iterp(time_t now) {
  sm_sock_t sock;
  chain_slot_t chs1, chs2;
  struct hostent *hent;
  struct sockaddr_in addr;
  struct in_addr *ia;
  int newfd, ret;

  chs1 = tsocks->first;
  while(chs1) {
    chs2 = chs1->next;
    sock = (sm_sock_t)chs1->v;
    ASSERT(!sock->connected, "not connected");
    if(now - sock->lat >= sock->cto) {
      chain_remove_slot(tsocks, chs1);
      ret = sm__close_h(sock->poll_fd);
      ASSERT(ret, "sm__close_h");
    }
    chs1 = chs2;
  }

  chs1 = csocks->first;
  while(chs1) {
    chs2 = chs1->next;
    sock = (sm_sock_t)chs1->v;
    chain_remove_slot(csocks, chs1);
    sock->autoconnect = 0;
    ret = sm__close_h(sock->poll_fd);
    ASSERT(ret, "sm__close_h");
    chs1 = chs2;
  }

  chs1 = wsocks->first;
  while(chs1) {
    chs2 = chs1->next;
    sock = (sm_sock_t)chs1->v;
    ASSERT(!sock->ch, "sock->ch is null");
    if(sock->poll_fd->fd >= 0) {
      if(now - sock->lrctt >= sock->ceto) {
        ret = sm__connect_h(sock->poll_fd, 1);
        ASSERT(ret, "sm__connect_h");
      }
    } else {
      if(now - sock->lrctt >= sock->rci) {
        sock->lrctt = now;

        hent = gethostbyname(sock->haddr);
        if(hent && (hent->h_length > 0)) {
          ia = (struct in_addr *)hent->h_addr_list[0];

          newfd = socket(AF_INET, SOCK_STREAM, 0);
          ASSERT(newfd<0, "socket");

          sm_set_fd_nb(newfd);

          addr.sin_family = AF_INET;
          addr.sin_addr.s_addr = ia->s_addr;
          addr.sin_port = htons(sock->hport);

          ret = connect(newfd, (struct sockaddr *)&addr, sizeof(addr));
          ASSERT(ret && (errno != EINPROGRESS), "connect");

          sock->poll_fd->fd = newfd;

          ret = poll_enable_fd(sock->poll_fd);
          ASSERT(ret, "poll_enable_fd");

          ret = poll_mod_fd(sock->poll_fd, 1);
          ASSERT(ret, "poll_mod_fd");
        }
      }
    }
    chs1 = chs2;
  }

  return 0;
 error:
  return -1;
}

sm_sock_t
sm_listen(int port) {
  sm_sock_t sock;
  struct sockaddr_in addr;
  int newfd, ret, v;

  newfd = socket(AF_INET, SOCK_STREAM, 0);
  ASSERT(newfd<0, "socket");

  v = 1;
  ret = setsockopt(newfd, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v));
  ASSERT(ret<0, "setsockopt");

  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;

  ret = bind(newfd, (struct sockaddr *)&addr, sizeof(addr));
  ASSERT(ret<0, "bind");

  ret = listen(newfd, 1);
  ASSERT(ret<0, "listen");

  sm_set_fd_nb(newfd);

  sock = sm_sock_new(SM_SOCK_TYPE_SERVER);
  ASSERT(!sock, "sm_sock_new");
  sock->connected = 1;
  sock->con_state = 1;
  strcpy(sock->saddr, "0.0.0.0");
  sock->sport = port;

  sock->poll_fd = poll_add_fd(newfd, sock, sm__read_h,
                              sm__write_h, sm__close_h);
  ASSERT(!sock->poll_fd, "poll_add_fd");

  return sock;
 error:
  return NULL;
}

sm_sock_t
sm_slisten(int port, char *cert, char *key) {
  sm_sock_t sock;
  int ret;

  sm_ssl_init();

  sock = sm_listen(port);
  ASSERT(!sock, "sm_listen");

  sock->secure = 1;

  sock->ssl_ctx = SSL_CTX_new(TLSv1_2_server_method());
  ASSERT(!sock->ssl_ctx, "SSL_CTX_new");

  ret = SSL_CTX_use_certificate_file(sock->ssl_ctx, cert, SSL_FILETYPE_PEM);
  ASSERT(!ret, "SSL_CTX_use_certificate_file");

  ret = SSL_CTX_use_PrivateKey_file(sock->ssl_ctx, key, SSL_FILETYPE_PEM);
  ASSERT(!ret, "SSL_CTX_use_PrivateKey_file");

  return sock;
 error:
  return NULL;
}

sm_sock_t
sm_connect(char *ip, int port, uint16_t rci) {
  struct in_addr iaddr;
  sm_sock_t sock;
  int ret;

  sock = sm_sock_new(SM_SOCK_TYPE_CONNECT);
  ASSERT(!sock, "sm_sock_new");
  sock->poll_fd = poll_fd_new();
  ASSERT(!sock->poll_fd, "poll_fd_new");
  sock->poll_fd->fd = -1;
  sock->poll_fd->ro = sock;
  sock->poll_fd->rh = sm__read_h;
  sock->poll_fd->wh = sm__write_h;
  sock->poll_fd->eh = sm__close_h;
  sock->autoconnect = rci ? 1 : 0;
  sock->con_state = 2;
  if(inet_aton(ip, &iaddr))
    sock->sa = (uint32_t)iaddr.s_addr;
  strcpy(sock->haddr, ip);
  sock->hport = port;
  sock->ceto = 3;
  sock->rci = rci;

  ret = chain_append(wsocks, OBJ(sock));
  ASSERT(ret, "chain_append");

  return sock;
 error:
  return NULL;
}

sm_sock_t
sm_sconnect(char *ip, int port, uint16_t rci) {
  sm_sock_t sock;

  sm_ssl_init();

  sock = sm_connect(ip, port, rci);
  ASSERT(!sock, "sm_connect");

  sock->secure = 1;

  sock->ssl_ctx = SSL_CTX_new(TLSv1_2_client_method());
  ASSERT(!sock->ssl_ctx, "SSL_CTX_new");

  sock->ssl = NULL;

  return sock;
 error:
  return NULL;
}

int
sm_set_cto(sm_sock_t sock, uint16_t cto) {
  chain_slot_t chs;
  int ret;

  ASSERT(!sock->connected, "socket not connected");

  chs = tsocks->first;
  while(chs) {
    if(((sm_sock_t)chs->v) == sock)
      break;
    chs = chs->next;
  }
  if(cto) {
    if(!chs) {
      ret = chain_append(tsocks, OBJ(sock));
      ASSERT(ret, "chain_append");
    }
  } else {
    if(chs) {
      chain_remove_slot(tsocks, chs);
    }
  }
  sock->cto = cto;
  time(&sock->lat);

  return 0;
 error:
  return -1;
}

int
sm_send(sm_sock_t sock, char *d, uint32_t ds) {
  int ret;

  ASSERT(!sock->connected, "socket not connected");

  if(sock->con_state != 3) {
    ret = str_add(sock->sbuf, d, ds);
    ASSERT(ret, "str_add");

    ret = poll_mod_fd(sock->poll_fd, 1);
    ASSERT(ret, "poll_mod_fd");

    if(sock->secure)
      sock->ssl_state = 4;

    if(sock->cto)
      time(&sock->lat);
  }

  return 0;
 error:
  return -1;
}

int
sm_close(sm_sock_t sock) {
  int ret;

  ASSERT(!sock->connected, "socket not connected");

  sock->con_state = 3;

  ret = chain_append(csocks, OBJ(sock));
  ASSERT(ret, "chain_append");

  return 0;
 error:
  return -1;
}

void
sm_set_fd_nb(int fd) {
  int flags;
	
  flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void
sm_set_fd_b(int fd) {
  int flags;
	
  flags = fcntl(fd, F_GETFL, 0);
  flags &= ~O_NONBLOCK;
  fcntl(fd, F_SETFL, flags);
}

static int
sm_set_fd_ka(int fd) {
  int ret, v;

  v = 1;
  ret = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &v, sizeof(v));
  ASSERT(ret<0, "setsockopt");
  v = KA_IDLE;
  ret = setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &v, sizeof(v));
  ASSERT(ret<0, "setsockopt");
  v = KA_INT;
  ret = setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &v, sizeof(v));
  ASSERT(ret<0, "setsockopt");
  v = KA_CNT;
  ret = setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &v, sizeof(v));
  ASSERT(ret<0, "setsockopt");

  return 0;
 error:
  return -1;
}

int
sm_get_fd_mac(sm_sock_t sock, char *dev, unsigned char **res) {
  struct arpreq ar;
  unsigned char *ptr;
  struct in_addr ipaddr;
  int ret;

  *res = _mac;

  memset(&ar, 0, sizeof(struct arpreq));
  ((struct sockaddr_in *) &ar.arp_pa)->sin_family = AF_INET;
  ret = inet_aton(sock->saddr, &ipaddr);
  ASSERT(ret == 0, "inet_aton");
  ((struct sockaddr_in *) &ar.arp_pa)->sin_addr = ipaddr;
  ((struct sockaddr_in *) &ar.arp_ha)->sin_family = ARPHRD_ETHER;
  strcpy(ar.arp_dev, dev);

  ret = ioctl(sock->poll_fd->fd, SIOCGARP, &ar);
  ASSERT((ret<0)&&(errno!=ENODEV)&&(errno!=ENXIO), "ioctl");
  if(ret < 0) {
    memset(_mac, 0, SM_MAC_LEN);
    return 1;
  } else {
    ptr = (unsigned char *)ar.arp_ha.sa_data;
    memcpy(_mac, ptr, SM_MAC_LEN);
  }

  return 0;
 error:
  return -1;
}

int
sm_generate_clients_uid(sm_sock_t sock, str_t dst) {
  struct in_addr ia;
  unsigned char *ap, *pp;
  uint32_t port;
  int ret;

  ASSERT(!sock->connected, "socket is not connected");

  memset(&ia, 0, sizeof(struct in_addr));

  ret = inet_aton(sock->saddr, &ia);
  ASSERT(!ret, "inet_aton");

  port = (uint32_t)sock->sport;

  ap = (unsigned char *)&ia.s_addr;
  pp = (unsigned char *)&port;

  ret = str_addf(dst, "%02x%02x%02x%02x%02x%02x%02x%02x",
                 pp[0], ap[1], pp[2], ap[3],
                 ap[0], pp[1], ap[2], pp[3]);
  ASSERT(ret, "str_addf");

  return 0;
 error:
  return -1;
}

static int
sm__accept_h(poll_fd_t poll_fd) {
  sm_sock_t sock, csock;
  struct sockaddr_in addr;
  socklen_t addr_l=0;
  int ret, newfd;

  sock = (sm_sock_t)poll_fd->ro;

  ASSERT(!sock->ch, "sock->ch is null");

  addr_l = sizeof(struct sockaddr_in);
  memset(&addr, 0, addr_l);
  newfd = accept(poll_fd->fd, (struct sockaddr *)&addr, (socklen_t *)&addr_l);
  if(newfd > 0) {
    sm_set_fd_nb(newfd);

    ret = sm_set_fd_ka(newfd);
    ASSERT(ret, "sm_set_fd_ka");

    csock = sm_sock_new(SM_SOCK_TYPE_CLIENT);
    ASSERT(!csock, "sm_sock_new");

    csock->ca = (uint32_t)addr.sin_addr.s_addr;
    strcpy(csock->saddr, inet_ntoa(addr.sin_addr));
    csock->sport = ntohl(addr.sin_port);
    addr_l = sizeof(struct sockaddr_in);
    memset(&addr, 0, addr_l);
    ret = getsockname(newfd, (struct sockaddr *)&addr, (socklen_t *)&addr_l);
    ASSERT(ret==-1, "getsockname()");
    csock->sa = (uint32_t)addr.sin_addr.s_addr;
    strcpy(csock->haddr, inet_ntoa(addr.sin_addr));
    csock->hport = ntohl(addr.sin_port);

    csock->poll_fd = poll_add_fd(newfd, csock, sm__read_h,
				 sm__write_h, sm__close_h);
    ASSERT(!csock->poll_fd, "poll_add_fd");

    if(sock->secure) {
      csock->secure = 1;
      csock->ssl = SSL_new(sock->ssl_ctx);
      ASSERT(!csock->ssl, "SSL_new");
      ret = SSL_set_fd(csock->ssl, newfd);
      ASSERT(!ret, "SSL_set_fd");
      csock->ssl_state = 1;
      csock->ch = sock->ch;
      ret = sm__sslAccept_h(csock);
      ASSERT(ret, "sm__sslAccept_h");
    } else {
      csock->connected = 1;
      ret = sock->ch(csock);
      ASSERT(ret, "sock->ch");
    }
  } else {
    ASSERT((errno != EAGAIN) && (errno != EWOULDBLOCK), "accept");
  }

  return 0;
 error:
  return -1;
}

static int
sm__sslAccept_h(sm_sock_t sock) {
  int ret;

  ASSERT(!sock->secure, "not ssl socket");
  ASSERT(sock->ssl_state!=1, "bad ssl_state");

  ret = SSL_accept(sock->ssl);
  if(!ret) {
    ret = SSL_get_error(sock->ssl, ret);
    if(ret == SSL_ERROR_WANT_READ) {
      ret = poll_mod_fd(sock->poll_fd, 0);
      ASSERT(ret, "poll_mod_fd");
    } else if(ret == SSL_ERROR_WANT_WRITE) {
      ret = poll_mod_fd(sock->poll_fd, 1);
      ASSERT(ret, "poll_mod_fd");
    } else {
      /* PWAR("SSL_accept: %s\n", ERR_error_string(ret, NULL)); */
      return sm__close_h(sock->poll_fd);
    }
  } else {
    ret = poll_mod_fd(sock->poll_fd, 0);
    ASSERT(ret, "poll_mod_fd");
    sock->ssl_state = 3;
    sock->connected = 1;
    ret = sock->ch(sock);
    ASSERT(ret, "sock->ch");
  }

  return 0;
 error:
  return -1;
}

static int
sm__connect_h(poll_fd_t poll_fd, int stop) {
  sm_sock_t sock;
  chain_slot_t sock_chs;
  struct sockaddr_in addr;
  socklen_t slen;
  int ret, addr_l, cfail;

  sock = (sm_sock_t)poll_fd->ro;

  sock_chs = wsocks->first;
  while(sock_chs) {
    if(((sm_sock_t)sock_chs->v) == sock)
      break;
    sock_chs = sock_chs->next;
  }
  ASSERT(!sock_chs, "sock not in 'wsocks'");

  if(stop) {
    cfail = 1;
  } else {
    slen = sizeof(cfail);
    ret = getsockopt(poll_fd->fd, SOL_SOCKET, SO_ERROR, &cfail, &slen);
    ASSERT(ret<0, "getsockopt");
  }

  if(cfail) {
    ret = sm__close_h(sock->poll_fd);
    ASSERT(ret, "sm__close_h");
  } else {
    chain_remove_slot(wsocks, sock_chs);

    ret = sm_set_fd_ka(poll_fd->fd);
    ASSERT(ret, "sm_set_fd_ka");

    sock->con_state = 0;

    addr_l = sizeof(struct sockaddr_in);
    memset(&addr, 0, addr_l);
    ret = getsockname(poll_fd->fd, (struct sockaddr *)&addr, (socklen_t *)&addr_l);
    ASSERT(ret==-1, "getsockname()");
    sock->ca = (uint32_t)addr.sin_addr.s_addr;
    strcpy(sock->saddr, inet_ntoa(addr.sin_addr));
    sock->sport = ntohl(addr.sin_port);

    ret = poll_mod_fd(poll_fd, 0);
    ASSERT(ret, "poll_mod_fd");

    if(sock->secure) {
      sock->ssl = SSL_new(sock->ssl_ctx);
      ASSERT(!sock->ssl, "SSL_new");
      ret = SSL_set_fd(sock->ssl, poll_fd->fd);
      ASSERT(!ret, "SSL_set_fd");
      sock->ssl_state = 2;
      ret = sm__sslConnect_h(sock);
      ASSERT(ret, "sm__sslAccept_h");
    } else {
      sock->connected = 1;
      ret = sock->ch(sock);
      ASSERT(ret, "sock->ch");
    }
  }

  return 0;
 error:
  return -1;
}

static int
sm__sslConnect_h(sm_sock_t sock) {
  int ret;

  ASSERT(!sock->secure, "not ssl socket");
  ASSERT(sock->ssl_state!=2, "bad ssl_state");

  ret = SSL_connect(sock->ssl);
  if(!ret) {
    ret = SSL_get_error(sock->ssl, ret);
    if(ret == SSL_ERROR_WANT_READ) {
      ret = poll_mod_fd(sock->poll_fd, 0);
      ASSERT(ret, "poll_mod_fd");
    } else if(ret == SSL_ERROR_WANT_WRITE) {
      ret = poll_mod_fd(sock->poll_fd, 1);
      ASSERT(ret, "poll_mod_fd");
    } else {
      /* PWAR("SSL_connect: %s\n", ERR_error_string(ret, NULL)); */
      return sm__close_h(sock->poll_fd);
    }
  } else {
    ret = poll_mod_fd(sock->poll_fd, 0);
    ASSERT(ret, "poll_mod_fd");
    sock->ssl_state = 3;
    sock->connected = 1;
    ret = sock->ch(sock);
    ASSERT(ret, "sock->ch");
  }

  return 0;
 error:
  return -1;
}

static int
sm__read_h(poll_fd_t poll_fd) {
  char rbuff[4096];
  sm_sock_t sock;
  int rc, ret;

  sock = (sm_sock_t)poll_fd->ro;

  if(sock->con_state != 3) {
    if(sock->type == SM_SOCK_TYPE_SERVER)
      return sm__accept_h(poll_fd);

    if(sock->cto)
      time(&sock->lat);

    ASSERT(!sock->rh, "sock->rh is null");

    if(sock->secure) {
      if(sock->ssl_state == 1)
        return sm__sslAccept_h(sock);
      else if(sock->ssl_state == 2)
        return sm__sslConnect_h(sock);
      else if(sock->ssl_state == 3)
        return sm__sslRead_h(sock);
      else if(sock->ssl_state == 4)
        return sm__sslWrite_h(sock);
    } else {
      while((rc = read(poll_fd->fd, rbuff, 4095)) > 0) {
        ret = sock->rh(sock, rbuff, rc);
        ASSERT(ret, "sock->rh");
      }
      if(rc == 0) {
        return sm__close_h(poll_fd);
      } else if(rc < 0) {
        ASSERT((errno != EAGAIN) && (errno != EWOULDBLOCK), "read");
      }
    }
  }
  
  return 0;
 error:
  return -1;
}

static int
sm__sslRead_h(sm_sock_t sock) {
  char rbuff[4096];
  int rc, ret;

  while((rc = SSL_read(sock->ssl, rbuff, 4095)) > 0) {
    ret = sock->rh(sock, rbuff, rc);
    ASSERT(ret, "sock->rh");
  }
  if(rc == 0) {
    return sm__close_h(sock->poll_fd);
  } else if(rc < 0) {
    ret = SSL_get_error(sock->ssl, rc);
    if(ret == SSL_ERROR_WANT_WRITE) {
      ret = poll_mod_fd(sock->poll_fd, 1);
      ASSERT(ret, "poll_mod_fd");
    } else if(ret != SSL_ERROR_WANT_READ) {
      /* PWAR("SSL_read:\n"); */
      /* PWAR("\t%d %s\n", ret, ERR_error_string(ret, NULL)); */
      /* while((ret = ERR_get_error())) { */
      /*   PWAR("\t%d %s\n", ret, ERR_error_string(ret, NULL)); */
      /* } */
      return sm__close_h(sock->poll_fd);
    }
  }

  return 0;
 error:
  return -1;
}

static int
sm__write_h(poll_fd_t poll_fd) {
  sm_sock_t sock;
  int ret, scnt;

  sock = (sm_sock_t)poll_fd->ro;

  if(sock->con_state != 3) {
    if(sock->con_state == 2)
      return sm__connect_h(poll_fd, 0);

    if(sock->cto)
      time(&sock->lat);

    if(sock->secure) {
      if(sock->ssl_state == 1)
        return sm__sslAccept_h(sock);
      else if(sock->ssl_state == 2)
        return sm__sslConnect_h(sock);
      else if(sock->ssl_state == 3)
        return sm__sslRead_h(sock);
      else if(sock->ssl_state == 4)
        return sm__sslWrite_h(sock);
    } else {
      if(sock->scount == sock->sbuf->l) {
        str_reset(sock->sbuf);
        sock->scount = 0;
        ret = poll_mod_fd(poll_fd, 0);
        ASSERT(ret, "poll_mod_fd");
        return 0;
      }
      scnt = write(poll_fd->fd, sock->sbuf->v+sock->scount, sock->sbuf->l-sock->scount);
      if(scnt <= 0) {
        ASSERT((errno != EAGAIN) && (errno != EWOULDBLOCK) && (errno != EBADF),
               "socket write");
      } else {
        sock->scount += scnt;
      }
    }
  }

  return 0;
 error:
  return -1;
}

static int
sm__sslWrite_h(sm_sock_t sock) {
  int ret, scnt;

  if(sock->scount == sock->sbuf->l) {
    str_reset(sock->sbuf);
    sock->scount = 0;
    ret = poll_mod_fd(sock->poll_fd, 0);
    ASSERT(ret, "poll_mod_fd");
    sock->ssl_state = 3;
    return 0;
  }

  scnt = SSL_write(sock->ssl, sock->sbuf->v+sock->scount, sock->sbuf->l-sock->scount);
  if(scnt > 0) {
    sock->scount += scnt;
  } else {
    ret = SSL_get_error(sock->ssl, scnt);
    if(ret == SSL_ERROR_WANT_READ) {
      ret = poll_mod_fd(sock->poll_fd, 0);
      ASSERT(ret, "poll_mod_fd");
    } else if(ret != SSL_ERROR_WANT_WRITE) {
      PWAR("SSL_write:\n");
      PWAR("\t%d %s\n", ret, ERR_error_string(ret, NULL));
      while((ret = ERR_get_error())) {
        PWAR("\t%d %s\n", ret, ERR_error_string(ret, NULL));
      }
      goto error;
    }
  }

  return 0;
 error:
  return -1;
}

static int
sm__close_h(poll_fd_t poll_fd) {
  sm_sock_t sock;
  chain_slot_t chs;
  int ret;

  sock = (sm_sock_t)poll_fd->ro;

  /* shutdown(poll_fd->fd, 2); */
  close(poll_fd->fd);

  if(sock->secure && sock->ssl_state) {
    SSL_shutdown(sock->ssl);
    SSL_free(sock->ssl);
    sock->ssl = NULL;
    sock->ssl_state = 0;
  }

  chs = wsocks->first;
  while(chs) {
    if(((sm_sock_t)chs->v) == sock) {
      chain_remove_slot(wsocks, chs);
      break;
    }
    chs = chs->next;
  }

  chs = tsocks->first;
  while(chs) {
    if(((sm_sock_t)chs->v) == sock) {
      chain_remove_slot(tsocks, chs);
      break;
    }
    chs = chs->next;
  }

  chs = csocks->first;
  while(chs) {
    if(((sm_sock_t)chs->v) == sock) {
      chain_remove_slot(csocks, chs);
      break;
    }
    chs = chs->next;
  }

  if((sock->type == SM_SOCK_TYPE_CONNECT) && sock->autoconnect) {
    SSL_CTX_free(sock->ssl_ctx);
    sock->ssl_ctx = SSL_CTX_new(TLSv1_2_client_method());
    ASSERT(!sock->ssl_ctx, "SSL_CTX_new");

    ret = poll_disable_fd(poll_fd);
    ASSERT(ret, "poll_disable_fd");

    if(sock->connected) {
      sock->connected = 0;
      ASSERT(!sock->eh, "sock->eh is null");
      ret = sock->eh(sock);
      ASSERT(ret, "sock->eh");
    }

    ret = chain_append(wsocks, OBJ(sock));
    ASSERT(ret, "chain_append");

    poll_fd->fd = -1;
    time(&sock->lrctt);
    sock->con_state = 2;
    str_reset(sock->sbuf);
  } else {
    sock->connected = 0;

    ret = poll_remove_fd(poll_fd);
    ASSERT(ret, "poll_remove_fd");

    ASSERT(!sock->eh, "sock->eh is null");
    ret = sock->eh(sock);
    ASSERT(ret, "sock->eh");

    if((sock->type == SM_SOCK_TYPE_CONNECT) && sock->secure)
      SSL_CTX_free(sock->ssl_ctx);

    sm_sock_destroy(sock);
  }

  return 0;
 error:
  return -1;
}

static sm_sock_t
sm_sock_new(sm_sock_type_e type) {
  sm_sock_t sock;

  sock = mem_alloc(sock_mc);
  ASSERT(!sock, "mem_alloc");

  memset(sock, 0, sizeof(struct sm_sock_st));

  sock->obj.type = OBJ_OBJECT;
  sock->type = type;

  sock->sbuf = str_new();
  ASSERT(!sock->sbuf, "str_new");
  str_2_byte(sock->sbuf);

  return sock;
 error:
  return NULL;
}

static void
sm_sock_destroy(sm_sock_t sock) {
  if(sock) {
    OBJ_DESTROY(sock->sbuf);
    mem_free(sock_mc, sock, 0);
  }
}
