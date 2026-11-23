// custo.c : multiplicacoes em F_p por permutacao (COUNT_OPS) x formula N(m C(a) + m C(a^-1) + 2m^2)
#include <inttypes.h>
#include <stdio.h>
#include "../src/rescue.h"
#include "../goldilocks/gl.c"

// square-and-multiply: floor(log2 e) quadrados + hw(e) - 1 produtos
static uint64_t custo_pow(uint64_t e){
    return (63 - __builtin_clzll(e)) + __builtin_popcountll(e) - 1;
}

int main(void){
    struct { const char* nome; uint64_t p; unsigned m, c, s; } tab[] = {
        { "Goldilocks",    0xFFFFFFFF00000001ULL, 12, 4, 128 },
        { "Mersenne-31",   (1ULL << 31) - 1,      12, 4, 128 },
        { "Mersenne-61",   (1ULL << 61) - 1,       8, 4, 128 },
        { "Fermat F4",     65537,                  4, 2,  64 },
        { "Pequeno",       101,                    3, 1,  16 },
        { "Maior 64 bits", UINT64_MAX - 58,        5, 2, 128 },
    };
    printf("%-14s %5s %7s %5s %5s %5s %8s %8s %7s\n",
           "", "C(a)", "C(a^-1)", "S", "S^-1", "MDS", "formula", "contado", "%S^-1");
    for (size_t k = 0; k < sizeof tab / sizeof tab[0]; k++){
        rp_params P = {0};
        if (rp_params_init(&P, tab[k].p, tab[k].m, tab[k].c, tab[k].s)){
            printf("rp_params_init falhou\n");
            return 1;
        }
        uint64_t m = P.m, ca = custo_pow(P.alpha), ci = custo_pow(P.alphainv);
        uint64_t rodada = m*ca + m*ci + 2*m*m;
        fe_t st[RP_MAX_M] = {0};
        fp_mul_count = 0;
        rp_permutation(&P, st);
        printf("%-14s %5" PRIu64 " %7" PRIu64 " %5" PRIu64 " %5" PRIu64 " %5" PRIu64 " %8" PRIu64 " %8" PRIu64 " %6.1f%%\n",
               tab[k].nome, ca, ci, m*ca, m*ci, 2*m*m, P.N * rodada, fp_mul_count, 100.0 * m*ci / rodada);
        rp_params_free(&P);
    }

    uint64_t st[GL_M] = {0};
    fp_mul_count = 0;
    gl_permutation(st);
    printf("\ngoldilocks especializado: %" PRIu64 " mults por permutacao (S^-1 = %.1f%%)\n",
           fp_mul_count, 100.0 * 12 * 72 / (12*4 + 12*72 + 2*144));
    return 0;
}
