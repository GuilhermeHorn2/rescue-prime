// params.c : geracao dos parametros do Rescue-Prime (passei do sage pra C)
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include "rescue.h"

static uint64_t gcd_u64(uint64_t a, uint64_t b){
    while (b){
        uint64_t t = a % b;
        a = b;
        b = t;
    }
    return a;
}

// euclides estendido, o modulo p-1 nao e primo entao Fermat nao serve
static uint64_t inv_mod(uint64_t a, uint64_t n){
    __int128 t = 0, newt = 1, r = n, newr = a;
    while (newr != 0){
        __int128 q = r / newr;
        __int128 tmp = t - q * newt;
        t = newt;
        newt = tmp;
        tmp = r - q * newr;
        r = newr;
        newr = tmp;
    }
    if (t < 0){
        t += n;
    }
    return (uint64_t)t;
}

static void get_alphas(uint64_t p, uint64_t* alpha, uint64_t* alphainv){
    uint64_t a = 3;
    while(gcd_u64(a, p-1) != 1){
        a++;
    }
    *alpha = a;
    *alphainv = inv_mod(a, p-1);
}

// log2 do binomial via lgamma, C(n,k) tem centenas de bits
static double log2_binom(double n, double k){
    return (lgamma(n+1) - lgamma(k+1) - lgamma(n - k + 1)) / log(2.0);
}

static unsigned get_number_of_rounds(unsigned m, unsigned c, unsigned s, uint64_t alpha){
    unsigned rate = m-c, l1;
    for (l1 = 1; l1 < 25; l1++){
        double v = (double)m * (l1-1) + rate;
        double dcon = floor(0.5 * (double)(alpha-1) * m * (l1-1)+2);
        if(2.0 * log2_binom(v+dcon, v) > (double)s){
            break;
        }
    }
    if (l1 == 25){
        l1 = 24; 
    }
    unsigned base = l1 > 5 ? l1 : 5;
    return (3*base + 1)/2; 
}

// fatoracao de p-1, pro elemento primitivo da MDS
static uint64_t mulmod(uint64_t a, uint64_t b, uint64_t n){
    return (__uint128_t)a * b % n;
}

static uint64_t powmod(uint64_t a, uint64_t e, uint64_t n){
    uint64_t r = 1 % n;
    a %= n;
    while(e != 0){
        if(e & 1) {
            r = mulmod(r, a, n);
        }
        e >>= 1;
        a = mulmod(a, a, n);
    }
    return r;
}

// Miller-Rabin, deterministico pra n < 2^64 com essas 12 bases
static int is_prime(uint64_t n){
    static const uint64_t bases[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};
    if (n < 2){
        return 0;
    }
    for (int i = 0; i < 12; i++){
        if (n % bases[i] == 0){
            return n == bases[i];
        }
    }
    uint64_t d = n - 1;
    int r = 0;
    while(!(d & 1)){
        d >>= 1;
        r++;
    }
    for (int i = 0; i < 12; i++){
        uint64_t x = powmod(bases[i], d, n);
        if (x == 1 || x == n-1){
            continue;
        }
        int ok = 0;
        for (int j = 1; j < r && !ok; j++){
            x = mulmod(x, x, n);
            ok = (x == n-1);
        }
        if (!ok){
            return 0;
        }
    }
    return 1;
}

// n composto
static uint64_t pollard_rho(uint64_t n){
    for(uint64_t c = 1;; c++){
        uint64_t x = 2;
        uint64_t y = 2;
        uint64_t d = 1;
        while (d == 1){
            x = (mulmod(x, x, n) + c) % n;
            y = (mulmod(y, y, n) + c) % n;
            y = (mulmod(y, y, n) + c) % n;
            d = gcd_u64(x > y ? x - y : y - x, n);
        }
        if (d != n){
            return d;
        }
    }
}

static void add_factor(uint64_t f, uint64_t* fs, int* nf){
    for (int i = 0; i < *nf; i++){
        if (fs[i] == f){
            return;
        }
    }
    fs[(*nf)++] = f;
}

static void factor_rec(uint64_t n, uint64_t* fs, int* nf){
    if (n == 1){
        return;
    }
    if(is_prime(n)){
        add_factor(n, fs, nf);
        return;
    }
    uint64_t d = pollard_rho(n);
    factor_rec(d, fs, nf);
    factor_rec(n / d, fs, nf);
}

static int prime_factors(uint64_t n, uint64_t* fs){
    int nf = 0;
    for (uint64_t q = 2; q < 1000 && q*q <= n; q++){
        if (n % q == 0){
            add_factor(q, fs, &nf);
            while(n % q == 0){
                n /= q;
            }
        }
    }
    factor_rec(n, fs, &nf);
    return nf;
}

static uint64_t primitive_element(uint64_t p){
    uint64_t fs[64];
    int nf = prime_factors(p-1, fs);
    for (uint64_t g = 2;; g++){
        int ok = 1;
        for (int i = 0; i < nf && ok; i++){
            ok = powmod(g, (p-1)/fs[i], p) != 1;
        }
        if (ok){
            return g;
        }
    }
}

// igual o Sage: vandermonde m x 2m, RREF, e a MDS e o lado direito transposto
static int get_mds_matrix(uint64_t p, unsigned m, fe_t* mds){
    unsigned w = 2*m;
    fe_t* V = malloc(sizeof(fe_t) * m * w);
    if (!V){
        return -1;
    }
    uint64_t g = primitive_element(p);

    for (unsigned i = 0; i < m; i++){
        for (unsigned j = 0; j < w; j++){
            V[i*w + j] = fp_pow(g % p, (uint64_t)i * j, p);
        }
    }

    unsigned r = 0;
    for (unsigned col = 0; col < w && r < m; col++){
        unsigned piv = r;
        while (piv < m && V[piv*w + col] == 0){
            piv++;
        }
        if (piv == m){
            continue;
        }
        for (unsigned j = 0; j < w; j++){
            fe_t t = V[r*w + j];
            V[r*w + j] = V[piv*w + j];
            V[piv*w + j] = t;
        }
        fe_t inv = fp_inv(V[r*w + col], p);
        for (unsigned j = 0; j < w; j++){
            V[r*w + j] = fp_mul(V[r*w + j], inv, p);
        }
        for (unsigned i = 0; i < m; i++){
            fe_t f = V[i*w + col];
            if (i == r || f == 0){
                continue;
            }
            for (unsigned j = 0; j < w; j++){
                V[i*w + j] = fp_sub(V[i*w + j], fp_mul(f, V[r*w + j], p), p);
            }
        }
        r++;
    }

    for (unsigned i = 0; i < m; i++){
        for (unsigned j = 0; j < m; j++){
            mds[i*m + j] = V[j*w + m + i];
        }
    }
    free(V);
    return 0;
}

// SHAKE-256 do Rescue-XLIX(p,m,c,s)
static int get_round_constants(const rp_params* P, fe_t* rc){
    unsigned bits = 64 - __builtin_clzll(P->p);
    size_t bpi = (bits + 7)/8 + 1; 
    size_t count = 2 * (size_t)P->m * P->N;
    size_t nbytes = bpi * count;
    char seed[128];
    snprintf(seed, sizeof seed, "Rescue-XLIX(%llu,%u,%u,%u)",
             (unsigned long long)P->p, P->m, P->c, P->s);

    unsigned char* buf = malloc(nbytes);
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    int ok = ctx && buf
          && EVP_DigestInit_ex(ctx, EVP_shake256(), NULL)
          && EVP_DigestUpdate(ctx, seed, strlen(seed))
          && EVP_DigestFinalXOF(ctx, buf, nbytes);
    EVP_MD_CTX_free(ctx);
    if (!ok){
        free(buf);
        return -1;
    }

    for (size_t i = 0; i < count; i++){
        __uint128_t x = 0;
        for (size_t j = bpi; j-- > 0;){ 
            x = (x << 8) | buf[i*bpi + j];
        }
        rc[i] = (fe_t)(x % P->p);
    }
    free(buf);
    return 0;
}

int rp_params_init(rp_params* P, uint64_t p, unsigned m, unsigned c, unsigned s){
    if (m > RP_MAX_M || c == 0 || c >= m || p < 5){
        return -1;
    }
    P->p = p;
    P->m = m;
    P->c = c;
    P->rate = m - c;
    P->s = s;
    get_alphas(p, &P->alpha, &P->alphainv);
    P->N = get_number_of_rounds(m, c, s, P->alpha);
    P->mds = malloc(sizeof(fe_t) * m * m);
    P->rc = malloc(sizeof(fe_t) * 2 * m * P->N);
    if (!P->mds || !P->rc || get_mds_matrix(p, m, P->mds) || get_round_constants(P, P->rc)){
        rp_params_free(P);
        return -1;
    }
    return 0;
}

void rp_params_free(rp_params* P){
    free(P->mds);
    free(P->rc);
    P->mds = P->rc = NULL;
}

#ifdef PARAMS_TEST

static int falhas = 0;
#define CHECK(cond) do { \
    if (!(cond)) { printf("FALHOU linha %d: %s\n", __LINE__, #cond); falhas++; } \
} while (0)

int main(){

    CHECK(gcd_u64(12, 18) == 6);
    CHECK(inv_mod(3, 100) == 67);
    CHECK(is_prime(65537) && is_prime(UINT64_MAX - 58) && !is_prime(65535) && !is_prime(1));

    uint64_t gl = 0xFFFFFFFF00000001ULL;
    uint64_t fs[64];
    int nf = prime_factors(gl - 1, fs);
    uint64_t prod = 1;
    for (int i = 0; i < nf; i++){
        CHECK(is_prime(fs[i]));
        prod *= fs[i];
    }
    CHECK(nf == 6 && prod == 2ULL*3*5*17*257*65537);

    nf = prime_factors(1000003ULL * 1000033ULL, fs);
    CHECK(nf == 2 && fs[0] * fs[1] == 1000003ULL * 1000033ULL);

    CHECK(primitive_element(gl) == 7);
    CHECK(primitive_element(17) == 3);

    // (p, m, c, s) -> alpha, alphainv, N
    struct { uint64_t p; unsigned m, c, s; uint64_t alpha, alphainv; unsigned N; } tab[] = {
        { gl,                    12, 4, 128,  7, 10540996611094048183ULL,  8 },
        { (1ULL << 61) - 1,       8, 4, 128, 17,  1763291712928118903ULL,  8 },
        { UINT64_MAX - 58,        5, 2, 128,  3, 12297829382473034371ULL, 12 },
        { (1ULL << 31) - 1,      12, 4, 128,  5,           1717986917ULL,  8 },
        { 65537,                  4, 2,  64,  3,                43691ULL,  8 },
        { 101,                    3, 1,  16,  3,                   67ULL,  8 },
    };
    for (size_t k = 0; k < sizeof tab / sizeof tab[0]; k++){
        rp_params P = {0};
        CHECK(rp_params_init(&P, tab[k].p, tab[k].m, tab[k].c, tab[k].s) == 0);
        CHECK(P.alpha == tab[k].alpha);
        CHECK(P.alphainv == tab[k].alphainv);
        CHECK(P.N == tab[k].N);
        CHECK(mulmod(P.alpha, P.alphainv, P.p - 1) == 1);
        fe_t x = 123456789 % P.p;
        CHECK(fp_pow(fp_pow(x, P.alpha, P.p), P.alphainv, P.p) == x);
        rp_params_free(&P);
    }

    // uns valores do Goldilocks conferidos no Sage
    rp_params P = {0};
    CHECK(rp_params_init(&P, gl, 12, 4, 128) == 0);
    CHECK(P.mds[0] == 2108866337646019936ULL);
    CHECK(P.mds[1] == 11223275256334781131ULL);
    CHECK(P.mds[2] == 2318414738826783588ULL);
    CHECK(P.rc[0] == 16089809142501829443ULL);
    CHECK(P.rc[1] == 3960375389654894755ULL);
    CHECK(P.rc[2] == 2341987601489900096ULL);

    
    for (unsigned i = 0; i < P.m * P.m; i++){
        CHECK(P.mds[i] != 0);
    }
    rp_params_free(&P);
    rp_params_free(&P);

    CHECK(rp_params_init(&P, gl, 4, 0, 128) != 0);
    CHECK(rp_params_init(&P, gl, 4, 4, 128) != 0);
    CHECK(rp_params_init(&P, gl, RP_MAX_M + 1, 4, 128) != 0);

    if (falhas == 0)
        printf("testes passaram\n");
    else
        printf("%d testes falharam\n", falhas);
    return falhas != 0;
}
#endif
