#include "objtypes.h"
#include "crypt.h"
#include "ns.h"
#include "trace.h"
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>

int
main(int argc, char **argv) {
  uint8_t mac[8], *p; // 84:8e:0c:38:a7:38
  uint64_t m;

  mac[0] = 0x00;
  mac[1] = 0x00;
  mac[2] = 0x84;
  mac[3] = 0x8e;
  mac[4] = 0x0c;
  mac[5] = 0x38;
  mac[6] = 0xa7;
  mac[7] = 0x38;
  m = ns_csu64(*((uint64_t *)mac));
  TRC("%ju\n", m);

  memset(mac, 0, 8);

  m = ns_csu64(m);
  p = (uint8_t *)&m;
  TR("%02x:%02x:%02x:%02x:%02x:%02x\n", p[2], p[3], p[4], p[5], p[6], p[7]);

  return 0;
}
