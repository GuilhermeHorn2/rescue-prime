// fp.c
#include "fp.h"
#ifdef COUNT_OPS
uint64_t fp_mul_count = 0;
#endif


// square-and-multiply da esquerda para a direita
fe_t fp_pow(fe_t a, uint64_t e, uint64_t p){
    if (e == 0){
        return 1 % p;
    }
    int i = 63 - __builtin_clzll(e);
    fe_t r = a;
    for (i = i - 1; i >= 0; i--){
        r = fp_mul(r, r, p);
        if ((e >> i) & 1){
            r = fp_mul(r, a, p);
        }
    }
    return r;
}


fe_t fp_inv(fe_t a, uint64_t p){
    return fp_pow(a, p - 2, p);
}


#ifdef FP_TEST
#include <stdio.h>

static int falhas = 0;
#define CHECK(cond) do { \
    if (!(cond)) { printf("FALHOU linha %d: %s\n", __LINE__, #cond); falhas++; } \
} while (0)

int main(){

    uint64_t q = 17;
    CHECK(fp_add(10, 9, q) == 2);
    CHECK(fp_sub(3, 5, q) == 15);
    CHECK(fp_mul(5, 7, q) == 1);
    CHECK(fp_pow(3, 0, q) == 1);
    CHECK(fp_pow(3, 4, q) == 13);
    CHECK(fp_inv(5, q) == 7);
    for (fe_t a = 1; a < q; a++){
        CHECK(fp_mul(a, fp_inv(a, q), q) == 1);
    }

    // primo grande, pra ver se o __uint128_t ta funcionando
    uint64_t p = UINT64_MAX - 58; // 2^64 - 59
    fe_t m = p - 1;
    CHECK(fp_add(m, m, p) == p - 2);
    CHECK(fp_add(m, 1, p) == 0);
    CHECK(fp_sub(0, 1, p) == m);
    CHECK(fp_mul(m, m, p) == 1);
    CHECK(fp_pow(m, 3, p) == m);
    CHECK(fp_pow(2, 64, p) == 59);

    fe_t xs[] = { 2, 3, 12345, 0xDEADBEEFCAFEBABEULL % p, m };
    for (int i = 0; i < 5; i++){
        fe_t a = xs[i];
        CHECK(fp_pow(a, p - 1, p) == 1);
        CHECK(fp_mul(a, fp_inv(a, p), p) == 1);
        CHECK(fp_sub(fp_add(a, 7, p), 7, p) == a);
    }

    if (falhas == 0)
        printf("testes passaram\n");
    else
        printf("%d testes falharam\n", falhas);
    return falhas != 0;
}
#endif
