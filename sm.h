#ifndef SM_HEADER
#define SM_HEADER

#include "obj.h"
#include <inttypes.h>
#include <time.h>
#include <openssl/ssl.h>

#define SM_MAC_LEN 6

typedef struct sm_sock_st *sm_sock_t;

typedef struct poll_fd_st *poll_fd_t;
typedef struct str_st *str_t;

typedef int (*sm_cc_handler_t) (sm_sock_t sock);
typedef int (*sm_r_handler_t) (sm_sock_t sock, char *d, int ds);
typedef int (*sm_udp_r_handler_t) (sm_sock_t sock, uint32_t sa, char *d, int ds);

typedef enum {
  SM_SOCK_TYPE_SERVER,
  SM_SOCK_TYPE_USERVER,
  SM_SOCK_TYPE_CLIENT,
  SM_SOCK_TYPE_CONNECT,
  SM_SOCK_TYPE_CUSTOM
} sm_sock_type_e;

struct sm_sock_st {
  struct obj_st obj;
  poll_fd_t poll_fd;
  sm_sock_type_e type;
  uint8_t connected;
  uint8_t reconnect;
  uint8_t con_state; // 0-established, 1-waitAccept, 2-waitConnect, 3-closing
  uint8_t secure;
  SSL_CTX *ssl_ctx;
  SSL *ssl;
  uint8_t ssl_state; // 0-new, 1-accept, 2-connect, 3-read, 4-write
  uint16_t ceto; // connection establishing timeout
  uint16_t rci; // reconnection interval
  time_t lrctt; // last connect-try time
  uint16_t cto; // connection timeout
  time_t lat; // last activity time
  uint32_t ca; // client address in bin
  uint32_t cp; // client port
  char cas[17]; // client address in string
  uint32_t sa; // server address in bin
  uint32_t sp; // server port
  char sas[17]; // server address in string
  str_t rsb;
  uint32_t scount;
  void *ro;
  sm_cc_handler_t ch;
  sm_r_handler_t rh;
  sm_udp_r_handler_t udp_rh;
  sm_cc_handler_t eh;
};


int
sm_init();

sm_sock_t
sm_listen(int port);

sm_sock_t
sm_slisten(int port, char *cert, char *key);

sm_sock_t
sm_udp_listen(int port, uint32_t bs);

sm_sock_t
sm_connect(char *ip, int port, uint16_t rci);

sm_sock_t
sm_sconnect(char *ip, int port, uint16_t rci);

int
sm_set_cto(sm_sock_t sock, uint16_t cto);

int
sm_send(sm_sock_t sock, char *d, uint32_t ds);

int
sm_udp_send(uint32_t da, uint32_t dp, char *d, uint32_t ds);

int
sm_reconnect(sm_sock_t sock);

int
sm_close(sm_sock_t sock);

uint32_t
sm_get_na4ip(char *ip);

uint32_t
sm_get_na4dn(char *dn);

char *
sm_get_ip4na(uint32_t na);

void
sm_set_fd_nb(int fd);

void
sm_set_fd_b(int fd);

int
sm_get_fd_mac(sm_sock_t sock, char *dev, unsigned char **res);

int
sm_generate_clients_uid(sm_sock_t sock, str_t dst);

#endif
