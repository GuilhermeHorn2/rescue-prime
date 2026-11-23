// gl.h : Rescue-Prime especializado pro Goldilocks
#ifndef GL_H
#define GL_H

#include <stddef.h>
#include <stdint.h>

#define GL_P 0xFFFFFFFF00000001ULL
#define GL_EPS 0xFFFFFFFFULL // 2^64 mod p = 2^32 - 1
#define GL_M 12
#define GL_RATE 8

void gl_permutation(uint64_t st[GL_M]);
void gl_hash(const uint64_t* in, size_t n, uint64_t out[GL_RATE]);

#endif
