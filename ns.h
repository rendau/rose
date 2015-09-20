#ifndef NS_HEADER
#define NS_HEADER

#include <inttypes.h>

#define ns_isne() (*((char *)&ns_ecv) == 0)

extern const uint16_t ns_ecv;

// unsigned integers:

uint16_t
ns_csu16(uint16_t n);

uint32_t
ns_csu32(uint32_t n);

uint64_t
ns_csu64(uint64_t n);

// signed integers:

int16_t
ns_csi16(int16_t n);

int32_t
ns_csi32(int32_t n);

int64_t
ns_csi64(int64_t n);

// float:

float
ns_csf(float n);

// double:

double
ns_csd(double n);

#endif
