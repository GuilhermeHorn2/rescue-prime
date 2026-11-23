// test_gl.c : a versao Goldilocks tem que dar o mesmo resultado da generica
// inclui o gl.c direto pra testar as funcoes static
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rescue.h"
#include "gl.c"

static int falhas = 0, total = 0;
#define CHECK(cond) do { total++; \
    if (!(cond)) { printf("    FALHOU linha %d: %s\n", __LINE__, #cond); falhas++; } \
} while (0)

static uint64_t rng_state = 2026;

static uint64_t rng(void){
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static int eq(const uint64_t* a, const uint64_t* b, size_t n){
    return memcmp(a, b, n * sizeof(uint64_t)) == 0;
}

static void teste_reducao(void){
    printf("reducao sem divisao\n");
    uint64_t p = GL_P;
    __uint128_t casos[] = {
        0, 1, p - 1, p, p + 1,
        (__uint128_t)(p - 1) * (p - 1), 
        (__uint128_t)(0xFFFFFFFFFFFFFFFFULL - 0xFFFFFFFFULL) * ((1ULL << 63) + 5), // cai no carry
        ((__uint128_t)1 << 64), ((__uint128_t)1 << 96), ((__uint128_t)1 << 127) + 3,
        ~(__uint128_t)0, 
    };
    int ok = 1;
    for (size_t i = 0; i < sizeof casos / sizeof casos[0]; i++){
        ok &= gl_reduce128(casos[i]) == (uint64_t)(casos[i] % p);
    }
    CHECK(ok);

   
    ok = 1;
    for (int k = 0; k < 1000000; k++){
        __uint128_t x = (__uint128_t)(rng() % p) * (rng() % p);
        __uint128_t y = ((__uint128_t)rng() << 64) | rng();
        ok &= gl_reduce128(x) == (uint64_t)(x % p);
        ok &= gl_reduce128(y) == (uint64_t)(y % p);
    }
    CHECK(ok);
}

static void teste_sbox(void){
    printf("sboxes x^7 e x^(7^-1)\n");
    uint64_t p = GL_P;
    uint64_t d = 10540996611094048183ULL;
    uint64_t xs[] = {0, 1, 2, p - 1, p - 2, GL_EPS, 1ULL << 32};
    int ok = 1;
    for (size_t i = 0; i < sizeof xs / sizeof xs[0]; i++){
        ok &= gl_pow7(xs[i]) == fp_pow(xs[i], 7, p);
        ok &= gl_pow_inv7(xs[i]) == fp_pow(xs[i], d, p);
    }
    for (int k = 0; k < 20000; k++){
        uint64_t x = rng() % p;
        ok &= gl_pow_inv7(x) == fp_pow(x, d, p);
        ok &= gl_pow7(gl_pow_inv7(x)) == x;
    }
    CHECK(ok);

#ifdef COUNT_OPS
    fp_mul_count = 0;
    gl_pow7(12345);
    CHECK(fp_mul_count == 4);
    fp_mul_count = 0;
    gl_pow_inv7(12345);
    CHECK(fp_mul_count == 72);
    fp_mul_count = 0;
    fp_pow(12345, d, p);
    CHECK(fp_mul_count == 95);
    printf("    mults no x^(7^-1) : generico 95, cadeia 72\n");
#endif
}

static void teste_diferencial(const rp_params* P){
    printf("gl contra a versao generica\n");
    uint64_t p = GL_P;
    uint64_t a[GL_M], b[GL_M];

    int ok = 1;
    for (int k = 0; k < 500; k++){
        for (int i = 0; i < GL_M; i++){
            if (k == 0){
                a[i] = 0;
            }
            else if (k == 1){
                a[i] = p - 1;
            }
            else{
                a[i] = rng() % p;
            }
        }
        memcpy(b, a, sizeof a);
        rp_permutation(P, a);
        gl_permutation(b);
        ok &= eq(a, b, GL_M);
    }
    CHECK(ok);

    // tamanhos 0 a 39 pegam todos os casos do padding
    uint64_t in[40], o1[GL_RATE], o2[GL_RATE];
    ok = 1;
    for (int k = 0; k < 1000; k++){
        size_t n = k < 40 ? (size_t)k : rng() % 40;
        for (size_t i = 0; i < n; i++){
            in[i] = rng() % p;
        }
        rp_hash(P, in, n, o1);
        gl_hash(in, n, o2);
        ok &= eq(o1, o2, GL_RATE);
    }
    CHECK(ok);

   
    for (int i = 0; i < 16; i++){
        in[i] = p - 1;
    }
    rp_hash(P, in, 16, o1);
    gl_hash(in, 16, o2);
    CHECK(eq(o1, o2, GL_RATE));
    for (int i = 0; i < 16; i++){
        in[i] = p + (rng() % GL_EPS);
    }
    rp_hash(P, in, 16, o1);
    gl_hash(in, 16, o2);
    CHECK(eq(o1, o2, GL_RATE));
}

static void teste_vetores(const char* arq){
    printf("vetores de referencia (%s)\n", arq);
    FILE* f = fopen(arq, "r");
    if (!f){
        perror(arq);
        falhas++;
        return;
    }
    char tag[16];
    uint64_t p = 0, a[GL_M], b[GL_M];
    int no_gl = 0, n_perm = 0, n_hash = 0, ok = 1;

    while (fscanf(f, "%15s", tag) == 1){
        if (!strcmp(tag, "PARAMS")){
            uint64_t m, c, s;
            if (fscanf(f, "%" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64, &p, &m, &c, &s) != 4){
                break;
            }
            no_gl = (p == GL_P && m == GL_M && c == GL_M - GL_RATE && s == 128);
        }
        else if (no_gl && !strcmp(tag, "PERM")){
            for (int i = 0; i < 2*GL_M; i++){
                if (fscanf(f, "%" SCNu64, i < GL_M ? &a[i] : &b[i - GL_M]) != 1){
                    ok = 0;
                }
            }
            gl_permutation(a);
            ok &= eq(a, b, GL_M);
            n_perm++;
        }
        else if (no_gl && !strcmp(tag, "HASH")){
            size_t n;
            if (fscanf(f, "%zu", &n) != 1){
                ok = 0;
                break;
            }
            uint64_t* in = malloc(sizeof(uint64_t) * (n + 1));
            for (size_t i = 0; i < n; i++){
                ok &= fscanf(f, "%" SCNu64, &in[i]) == 1;
            }
            for (int i = 0; i < GL_RATE; i++){
                ok &= fscanf(f, "%" SCNu64, &b[i]) == 1;
            }
            gl_hash(in, n, a);
            ok &= eq(a, b, GL_RATE);
            free(in);
            n_hash++;
        }
        else{
            int ch;
            while ((ch = fgetc(f)) != '\n' && ch != EOF){
            }
        }
    }
    fclose(f);
    CHECK(n_perm > 0 && n_hash > 0);
    CHECK(ok);
    printf("    %d PERM e %d HASH do Goldilocks\n", n_perm, n_hash);
}

int main(int argc, char** argv){
    const char* arq = argc > 1 ? argv[1] : "../ref/vectors_py.txt";
    rp_params P = {0};
    if (rp_params_init(&P, GL_P, GL_M, GL_M - GL_RATE, 128)){
        printf("rp_params_init falhou\n");
        return 1;
    }
    CHECK(P.N == GL_N);

    teste_reducao();
    teste_sbox();
    teste_diferencial(&P);
    teste_vetores(arq);

    rp_params_free(&P);
    if (falhas == 0){
        printf("todos os %d testes passaram\n", total);
    }
    else{
        printf("%d de %d testes falharam\n", falhas, total);
    }
    return falhas != 0;
}
