#include "ns.h"
#include <byteswap.h>

const uint16_t ns_ecv = 1;

// unsigned integers:

uint16_t
ns_csu16(uint16_t n) {
  if(ns_isne())
    return n;
  return __bswap_16(n);
}

uint32_t
ns_csu32(uint32_t n) {
  if(ns_isne())
    return n;
  return __bswap_32(n);
}

uint64_t
ns_csu64(uint64_t n) {
  if(ns_isne())
    return n;
  return __bswap_64(n);
}

// signed integers:

int16_t
ns_csi16(int16_t n) {
  if(ns_isne())
    return n;
  return __bswap_16(n);
}

int32_t
ns_csi32(int32_t n) {
  if(ns_isne())
    return n;
  return __bswap_32(n);
}

int64_t
ns_csi64(int64_t n) {
  if(ns_isne())
    return n;
  return __bswap_64(n);
}

// float:

float
ns_csf(float n) {
  unsigned char buff[4], *ptr;

  if(ns_isne())
    return n;

  ptr = (unsigned char *)&n;
  buff[0] = ptr[3]; buff[1] = ptr[2];
  buff[2] = ptr[1]; buff[3] = ptr[0];

  return *((float *)buff);
}

// double:

double
ns_csd(double n) {
  unsigned char buff[8], *ptr;

  if(ns_isne())
    return n;

  ptr = (unsigned char *)&n;
  buff[0] = ptr[7]; buff[1] = ptr[6];
  buff[2] = ptr[5]; buff[3] = ptr[4];
  buff[4] = ptr[3]; buff[5] = ptr[2];
  buff[6] = ptr[1]; buff[7] = ptr[0];

  return *((double *)buff);
}
