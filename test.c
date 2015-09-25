#include "objtypes.h"
#include "poll.h"
#include "sm.h"
#include "usm.h"
#include "trace.h"
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>

int
read_h(usm_sock_t sock, uint32_t sa, char *d, int ds) {
  TR("read_h: %d '%.*s'\n", ds, ds, d);
  return 0;
}

int
main(int argc, char **argv) {
  usm_sock_t usm_sock;
  int ret;

  ret = usm_init();
  ASSERT(ret, "usm_init");

  usm_sock = usm_listen(7676, 50);
  ASSERT(!usm_sock, "usm_listen");
  usm_sock->rh = read_h;

  ret = usm_send(sm_get_na4ip("127.0.0.1"), 7676, "hello from client!", 18);
  ASSERT(ret, "usm_send");

  while(1) {
    ret = poll_run(2000);
    ASSERT(ret, "poll_run");
  }

  return 0;
 error:
  return -1;
}
