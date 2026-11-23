
#include <string.h>
#include "fp.h"
#include "gl.h"
#include "gl_tables.h"

// pra ablacao: -DGL_SEM_REDUCAO (volta pro %), -DGL_SEM_CADEIA (square-and-multiply),
// -DGL_SEM_MDS_LAZY (MDS reduz a cada soma)


static inline uint64_t gl_reduce128(__uint128_t x){
    uint64_t lo = (uint64_t)x;
    uint64_t hi = (uint64_t)(x >> 64);
    uint64_t hh = hi >> 32;
    uint64_t hl = hi & GL_EPS;
    uint64_t t, r;

    if (__builtin_sub_overflow(lo, hh, &t)){
        t -= GL_EPS;
    }
    if (__builtin_add_overflow(t, hl * GL_EPS, &r)){
        r += GL_EPS;
    }
    return r >= GL_P ? r - GL_P : r;
}

static inline uint64_t gl_mul(uint64_t a, uint64_t b){
    COUNT_MUL();
#ifdef GL_SEM_REDUCAO
    return (uint64_t)(((__uint128_t)a * b) % GL_P);
#else
    return gl_reduce128((__uint128_t)a * b);
#endif
}

static inline uint64_t gl_sqr(uint64_t a){
    return gl_mul(a, a);
}

static inline uint64_t gl_pow7(uint64_t x){
    uint64_t x2 = gl_sqr(x);
    uint64_t x3 = gl_mul(x2, x);
    uint64_t x6 = gl_sqr(x3);
    return gl_mul(x6, x);
}


static inline uint64_t exp_acc(uint64_t base, uint64_t tail, int k){
    while (k--){
        base = gl_sqr(base);
    }
    return gl_mul(base, tail);
}


static inline uint64_t gl_pow_inv7(uint64_t x){
    uint64_t t1 = gl_sqr(x); 
    uint64_t t2 = gl_sqr(t1); 
    uint64_t t3 = exp_acc(t2, t2, 3); 
    uint64_t t4 = exp_acc(t3, t3, 6); 
    uint64_t t5 = exp_acc(t4, t4, 12);
    uint64_t t6 = exp_acc(t5, t3, 6); 
    uint64_t t7 = exp_acc(t6, t6, 31); 
    uint64_t a = gl_sqr(gl_sqr(gl_mul(gl_sqr(t7), t6)));
    uint64_t b = gl_mul(gl_mul(t1, t2), x); 
    return gl_mul(a, b);
}

static inline uint64_t gl_pow_sm(uint64_t a, uint64_t e){
    int i = 63 - __builtin_clzll(e);
    uint64_t r = a;
    for (i = i - 1; i >= 0; i--){
        r = gl_sqr(r);
        if ((e >> i) & 1){
            r = gl_mul(r, a);
        }
    }
    return r;
}

static inline uint64_t gl_sbox_inv(uint64_t x){
#ifdef GL_SEM_CADEIA
    return gl_pow_sm(x, 10540996611094048183ULL);
#else
    return gl_pow_inv7(x);
#endif
}

static inline void gl_mds(uint64_t st[GL_M]){
    uint64_t t[GL_M];
    for (int i = 0; i < GL_M; i++){
#ifdef GL_SEM_MDS_LAZY
        uint64_t acc = 0;
        for (int j = 0; j < GL_M; j++){
            acc = fp_add(acc, gl_mul(GL_MDS[i][j], st[j]), GL_P);
        }
        t[i] = acc;
#else
        __uint128_t acc = 0;
        for (int j = 0; j < GL_M; j++){
            acc += gl_mul(GL_MDS[i][j], st[j]);
        }
#ifdef GL_SEM_REDUCAO
        t[i] = (uint64_t)(acc % GL_P);
#else
        t[i] = gl_reduce128(acc);
#endif
#endif
    }
    memcpy(st, t, sizeof t);
}

void gl_permutation(uint64_t st[GL_M]){
    for (int r = 0; r < GL_N; r++){
        for (int j = 0; j < GL_M; j++){
            st[j] = gl_pow7(st[j]);
        }
        gl_mds(st);
        for (int j = 0; j < GL_M; j++){
            st[j] = fp_add(st[j], GL_RC[2*r][j], GL_P);
        }
        for (int j = 0; j < GL_M; j++){
            st[j] = gl_sbox_inv(st[j]);
        }
        gl_mds(st);
        for (int j = 0; j < GL_M; j++){
            st[j] = fp_add(st[j], GL_RC[2*r + 1][j], GL_P);
        }
    }
}

void gl_hash(const uint64_t* in, size_t n, uint64_t out[GL_RATE]){
    uint64_t st[GL_M] = {0};
    size_t total = ((n + 1 + GL_RATE - 1) / GL_RATE) * GL_RATE;

    for (size_t k = 0; k < total; k += GL_RATE){
        for (size_t i = 0; i < GL_RATE; i++){
            size_t idx = k + i;
            uint64_t x = idx < n ? in[idx] : (idx == n ? 1 : 0);
            if (x >= GL_P){
                x -= GL_P;
            }
            st[i] = fp_add(st[i], x, GL_P);
        }
        gl_permutation(st);
    }
    memcpy(out, st, sizeof(uint64_t) * GL_RATE);
}
