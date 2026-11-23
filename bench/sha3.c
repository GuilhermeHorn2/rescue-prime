// sha3.c : hash generico x goldilocks x SHA3-256 do OpenSSL pro mesmo tamanho de mensagem
#define _GNU_SOURCE
#include <stdlib.h>
#include <openssl/evp.h>
#include "perfctr.h"
#include "../src/rescue.h"
#include "../goldilocks/gl.h"

#define NMAX 1023

static rp_params P;
static uint64_t msg[NMAX];
static uint64_t out[GL_RATE];
static unsigned char dig[32];
static EVP_MD_CTX* ctx;
static size_t n_atual;
static volatile uint64_t lixo;

static void h_gen(int n){
    for (int k = 0; k < n; k++){
        rp_hash(&P, msg, n_atual, out);
        msg[0] = out[0];
    }
    lixo += out[0];
}

static void h_gl(int n){
    for (int k = 0; k < n; k++){
        gl_hash(msg, n_atual, out);
        msg[0] = out[0];
    }
    lixo += out[0];
}

static void h_sha3(int n){
    unsigned int len;
    for (int k = 0; k < n; k++){
        EVP_DigestInit_ex(ctx, EVP_sha3_256(), NULL);
        EVP_DigestUpdate(ctx, msg, 8 * n_atual);
        EVP_DigestFinal_ex(ctx, dig, &len);
        msg[0] ^= dig[0];
    }
    lixo += dig[0];
}

int main(void){
    if (rp_params_init(&P, GL_P, GL_M, GL_M - GL_RATE, 128)){
        printf("rp_params_init falhou\n");
        return 1;
    }
    ctx = EVP_MD_CTX_new();
    pc_init();
    for (int i = 0; i < NMAX; i++){
        msg[i] = (0x9E3779B97F4A7C15ULL * (i + 7)) % GL_P;
    }

    size_t tam[3] = {7, 63, 1023};
    printf("%6s %6s %5s | %10s %10s %10s | %8s %8s %8s\n", "elem", "bytes", "perm",
           "gen ciclos", "gl ciclos", "sha3 cic", "gen c/B", "gl c/B", "sha3 c/B");
    for (int i = 0; i < 3; i++){
        n_atual = tam[i];
        int reps = tam[i] < 100 ? 200 : 20;
        Medida g = mede(h_gen, reps);
        Medida l = mede(h_gl, reps);
        Medida s = mede(h_sha3, tam[i] < 100 ? 20000 : 2000);
        double bytes = 8.0 * tam[i];
        printf("%6zu %6.0f %5zu | %10.0f %10.0f %10.0f | %8.1f %8.1f %8.1f\n", tam[i], bytes, (tam[i] + 1 + 7) / 8,
               g.cic, l.cic, s.cic, g.cic / bytes, l.cic / bytes, s.cic / bytes);
    }

    EVP_MD_CTX_free(ctx);
    rp_params_free(&P);
    return lixo == 42;
}
