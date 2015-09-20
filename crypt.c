#include "crypt.h"
#include "objtypes.h"
#include "trace.h"
#include <stdlib.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>

static char sha1_digest_hex[CRYPT_SHA1_HDS+1];

unsigned char *
crypt_sha1_enc(char *data, uint32_t dlen) {
  return SHA1((const unsigned char *)data, dlen, NULL);
}

char *
crypt_sha1_enc_hex(char *data, uint32_t dlen) {
  unsigned char *d;
  int i;

  d = crypt_sha1_enc(data, dlen);

  for(i=0; i<CRYPT_SHA1_DS; i++)
    sprintf(sha1_digest_hex+i*2, "%02x", d[i]);

  sha1_digest_hex[CRYPT_SHA1_HDS] = 0;

  return sha1_digest_hex;
}

int
crypt_base64_enc(str_t res, char *data, uint32_t dlen) {
  BIO *bmem, *b64;
  BUF_MEM *bptr;
  int ret;

  b64 = BIO_new(BIO_f_base64());
  ASSERT(!b64, "BIO_new()");
  bmem = BIO_new(BIO_s_mem());
  ASSERT(!bmem, "BIO_new()");
  b64 = BIO_push(b64, bmem);
  ASSERT(!b64, "BIO_push()");
  ret = BIO_write(b64, data, dlen);
  ASSERT(ret<=0, "BIO_write()");
  ret = BIO_flush(b64);
  ASSERT(ret==-1, "BIO_flush()");
  BIO_get_mem_ptr(b64, &bptr);
  ret = str_set_val(res, bptr->data, bptr->length-1);
  ASSERT(ret!=0, "str_set()");
  BIO_free_all(b64);

  return 0;
 error:
  return -1;
}

int
crypt_rand_bytes(unsigned char *buf, int bsize) {
  if(RAND_bytes(buf, bsize) == 1)
    return 0;
  return -1;
}
