#include "objtypes.h"
#include "poll.h"
#include "sm.h"
#include "trace.h"
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>

int
read_h(sm_sock_t sock, uint32_t sa, char *d, int ds) {
  TR("read_h: (%s) '%.*s'\n", sm_get_ip4na(sa), ds, d);
  return 0;
}

int
main(int argc, char **argv) {
  sm_sock_t sm_sock;
  int ret;

  ret = sm_init();
  ASSERT(ret, "sm_init");

  sm_sock = sm_udp_listen(7676, 50);
  ASSERT(!sm_sock, "sm_listen");
  sm_sock->udp_rh = read_h;

  ret = sm_udp_send(sm_get_na4ip("127.0.0.1"), 7676, "hello from client!", 18);
  ASSERT(ret, "sm_udp_send");

  while(1) {
    ret = poll_run(2000);
    ASSERT(ret, "poll_run");
  }

  return 0;
 error:
  return -1;
}
