// rescue.h
#ifndef RESCUE_H
#define RESCUE_H

#include <stddef.h>
#include "fp.h"

#define RP_MAX_M 32 // largura maxima, assim o estado cabe na pilha

typedef struct {
    uint64_t p;
    unsigned m;
    unsigned c;
    unsigned rate; // m - c
    unsigned s; // seguranca em bits
    uint64_t alpha;
    uint64_t alphainv; // alpha^-1 mod (p-1)
    unsigned N; // rounds
    fe_t* mds; // m x m, linha por linha
    fe_t* rc; // 2*m*N constantes
} rp_params;

int rp_params_init(rp_params* P, uint64_t p, unsigned m, unsigned c, unsigned s);
void rp_params_free(rp_params* P);
void rp_permutation(const rp_params* P, fe_t state[]);
void rp_hash(const rp_params* P, const fe_t* in, size_t n, fe_t* out);

#endif
