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
  ASSERT(!str, "str_new");

  ret = crypt_base64_dec(str, "aGVsbG8gYmFzZTY0IQ==", 0);
  ASSERT(ret, "crypt_base64_dec");

  TRC("%s\n", str->v);

  return 0;
 error:
  return -1;
}
