#ifndef CRYPT_HEADER
#define CRYPT_HEADER

#include <inttypes.h>

#define CRYPT_SHA1_DS 20
#define CRYPT_SHA1_HDS 40

typedef struct str_st *str_t;

unsigned char *
crypt_sha1_enc(char *data, uint32_t dlen);

char *
crypt_sha1_enc_hex(char *data, uint32_t dlen);

int
crypt_base64_enc(str_t res, char *data, uint32_t dlen);

int
crypt_rand_bytes(unsigned char *buf, int bsize);

#endif
