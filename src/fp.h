// fp.h : aritmetica em F_p, p primo < 2^64
#ifndef FP_H
#define FP_H

#include <stdint.h>

typedef uint64_t fe_t; // elemento de F_p, sempre em [0, p)

#ifdef COUNT_OPS
extern uint64_t fp_mul_count;
#define COUNT_MUL() (fp_mul_count++)
#else
#define COUNT_MUL() ((void)0)
#endif

static inline fe_t fp_add(fe_t a, fe_t b, uint64_t p){
    __uint128_t s = (__uint128_t)a + b; // com p perto de 2^64 a soma estoura 64 bits
    return (fe_t)(s >= p ? s - p : s);
}

static inline fe_t fp_sub(fe_t a, fe_t b, uint64_t p){
    return a >= b ? a - b : a + (p - b);
}

static inline fe_t fp_mul(fe_t a, fe_t b, uint64_t p) {
    COUNT_MUL();
    return (fe_t)(((__uint128_t)a * b) % p); 
}

fe_t fp_pow(fe_t a, uint64_t e, uint64_t p);
fe_t fp_inv(fe_t a, uint64_t p);

#endif