#include "objtypes.h"
#include "crypt.h"
#include "trace.h"
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>

int
main(int argc, char **argv) {
  str_t str;
  int ret;

  str = str_new();

  ret = str_write_file(str, "out.txt");
  ASSERT(ret, "str_write_file");

  return 0;
 error:
  return -1;
}
