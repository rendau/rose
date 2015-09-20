#include "objtypes.h"
#include "spm.h"
#include "trace.h"
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>

#include <time.h>

int
main(int argc, char **argv) {
  time_t now;

  time(&now);

  TRC("%jd\n", now);

  return 0;
 /* error: */
 /*  return -1; */
}
