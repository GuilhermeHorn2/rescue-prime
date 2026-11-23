// test_rescue.c : parte A compara com os vetores do Sage, parte B testa propriedades
// uso: ./test_rescue ../ref/vectors.txt
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rescue.h"

static int falhas = 0, total = 0;
#define CHECK(cond) do { total++; \
    if (!(cond)) { printf("    FALHOU linha %d: %s\n", __LINE__, #cond); falhas++; } \
} while (0)

static FILE* vf;

static uint64_t rd(void){
    uint64_t x;
    if (fscanf(vf, "%" SCNu64, &x) != 1){
        fprintf(stderr, "arquivo de vetores mal formado\n");
        exit(2);
    }
    return x;
}

static void rd_vec(fe_t* v, size_t n){
    for (size_t i = 0; i < n; i++){
        v[i] = rd();
    }
}

static int eq(const fe_t* a, const fe_t* b, size_t n){
    return memcmp(a, b, n * sizeof(fe_t)) == 0;
}

// splitmix64 com seed fixa
static uint64_t rng_state = 2026;

static uint64_t rng(void){
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static fe_t rnd_fe(uint64_t p){
    return rng() % p; 
}

static void matvec(const fe_t* M, const fe_t* x, fe_t* y, unsigned m, uint64_t p){
    for (unsigned i = 0; i < m; i++){
        fe_t acc = 0;
        for (unsigned j = 0; j < m; j++){
            acc = fp_add(acc, fp_mul(M[i*m + j], x[j], p), p);
        }
        y[i] = acc;
    }
}

// gauss-jordan em [M | I], -1 se for singular
static int mat_inv(const fe_t* M, fe_t* Minv, unsigned m, uint64_t p){
    unsigned w = 2*m;
    fe_t A[RP_MAX_M * 2 * RP_MAX_M];
    for (unsigned i = 0; i < m; i++){
        for (unsigned j = 0; j < m; j++){
            A[i*w + j] = M[i*m + j];
            A[i*w + m + j] = (i == j);
        }
    }
    for (unsigned col = 0; col < m; col++){
        unsigned piv = col;
        while (piv < m && A[piv*w + col] == 0){
            piv++;
        }
        if (piv == m){
            return -1;
        }
        for (unsigned j = 0; j < w; j++){
            fe_t t = A[col*w + j];
            A[col*w + j] = A[piv*w + j];
            A[piv*w + j] = t;
        }
        fe_t inv = fp_inv(A[col*w + col], p);
        for (unsigned j = 0; j < w; j++){
            A[col*w + j] = fp_mul(A[col*w + j], inv, p);
        }
        for (unsigned i = 0; i < m; i++){
            fe_t f = A[i*w + col];
            if (i == col || f == 0){
                continue;
            }
            for (unsigned j = 0; j < w; j++){
                A[i*w + j] = fp_sub(A[i*w + j], fp_mul(f, A[col*w + j], p), p);
            }
        }
    }
    for (unsigned i = 0; i < m; i++){
        for (unsigned j = 0; j < m; j++){
            Minv[i*m + j] = A[i*w + m + j];
        }
    }
    return 0;
}

static void pow_all(fe_t* st, unsigned m, uint64_t e, uint64_t p){
    for (unsigned j = 0; j < m; j++){
        st[j] = fp_pow(st[j], e, p);
    }
}

static void add_all(fe_t* st, const fe_t* k, unsigned m, uint64_t p, int sinal){
    for (unsigned j = 0; j < m; j++){
        st[j] = sinal > 0 ? fp_add(st[j], k[j], p) : fp_sub(st[j], k[j], p);
    }
}

static void mat_all(const fe_t* M, fe_t* st, unsigned m, uint64_t p){
    fe_t t[RP_MAX_M];
    matvec(M, st, t, m, p);
    memcpy(st, t, m * sizeof(fe_t));
}

// desfaz os 6 passos de cada round de tras pra frente
static void perm_inv(const rp_params* P, const fe_t* Minv, fe_t st[]){
    unsigned m = P->m;
    uint64_t p = P->p;
    for (unsigned i = P->N; i-- > 0;){
        add_all(st, P->rc + 2*m*i + m, m, p, -1);
        mat_all(Minv, st, m, p);
        pow_all(st, m, P->alpha, p);
        add_all(st, P->rc + 2*m*i, m, p, -1);
        mat_all(Minv, st, m, p);
        pow_all(st, m, P->alphainv, p);
    }
}

// se o TRACE falhar, acha o primeiro passo do round 0 que diverge do Sage
static void diagnostico_trace(const rp_params* P, const fe_t* tr){
    unsigned m = P->m;
    uint64_t p = P->p;
    fe_t st[RP_MAX_M];
    const char* nome[6] = {"x^alpha", "MDS", "+K_0", "x^(1/alpha)", "MDS", "+K_1"};
    memcpy(st, tr, m * sizeof(fe_t));

    for (int k = 0; k < 6; k++){
        if (k == 0){
            pow_all(st, m, P->alpha, p);
        }
        else if (k == 3){
            pow_all(st, m, P->alphainv, p);
        }
        else if (k == 1 || k == 4){
            mat_all(P->mds, st, m, p);
        }
        else if (k == 2){
            add_all(st, P->rc, m, p, 1);
        }
        else{
            add_all(st, P->rc + m, m, p, 1);
        }

        if (!eq(st, tr + (k + 1) * m, m)){
            printf("    -> primeiro passo diferente do Sage: %d (%s)\n", k + 1, nome[k]);
            return;
        }
    }
    printf("    -> passos batem com o Sage; o erro esta no rp_permutation\n");
}

static void propriedades(const rp_params* P){
    unsigned m = P->m, r = P->rate;
    uint64_t p = P->p;
    fe_t Minv[RP_MAX_M * RP_MAX_M], I[RP_MAX_M], col[RP_MAX_M];
    fe_t x[RP_MAX_M], y[RP_MAX_M], z[RP_MAX_M];
    int ok;

    CHECK(mat_inv(P->mds, Minv, m, p) == 0);
    ok = 1;
    for (unsigned j = 0; j < m; j++){
        for (unsigned i = 0; i < m; i++){
            col[i] = Minv[i*m + j];
        }
        matvec(P->mds, col, I, m, p);
        for (unsigned i = 0; i < m; i++){
            ok &= (I[i] == (i == j));
        }
    }
    CHECK(ok);

    ok = 1;
    int menor_p = 1;
    for (int k = 0; k < 50; k++){
        for (unsigned i = 0; i < m; i++){
            x[i] = rnd_fe(p);
        }
        memcpy(y, x, m * sizeof(fe_t));
        rp_permutation(P, y);
        for (unsigned i = 0; i < m; i++){
            menor_p &= (y[i] < p);
        }
        perm_inv(P, Minv, y);
        ok &= eq(x, y, m);
    }
    CHECK(ok);
    CHECK(menor_p);

    for (unsigned i = 0; i < m; i++){
        x[i] = rnd_fe(p);
    }
    memcpy(y, x, m * sizeof(fe_t));
    rp_permutation(P, y);
    ok = 1;
    for (unsigned i = 0; i < m; i++){
        memcpy(z, x, m * sizeof(fe_t));
        z[i] = fp_add(z[i], 1, p);
        rp_permutation(P, z);
        ok &= !eq(y, z, m);
    }
    CHECK(ok);

    fe_t in[4 * RP_MAX_M], in_copia[4 * RP_MAX_M], o1[RP_MAX_M], o2[RP_MAX_M];
    size_t n = 3*r + 1;
    for (size_t i = 0; i < n; i++){
        in[i] = rnd_fe(p);
    }
    memcpy(in_copia, in, n * sizeof(fe_t));
    rp_hash(P, in, n, o1);
    rp_hash(P, in, n, o2);
    CHECK(eq(in, in_copia, n));
    CHECK(eq(o1, o2, r));
    ok = 1;
    for (unsigned i = 0; i < r; i++){
        ok &= (o1[i] < p);
    }
    CHECK(ok);

    ok = 1;
    for (size_t len = 0; len <= 2*r; len++){
        for (size_t i = 0; i < len; i++){
            in[i] = rnd_fe(p);
        }
        in[len] = 0;
        rp_hash(P, in, len, o1);
        rp_hash(P, in, len + 1, o2);
        ok &= !eq(o1, o2, r);
    }
    CHECK(ok);

    if (p < (1ULL << 63)){
        for (size_t i = 0; i < r; i++){
            in[i] = rnd_fe(p);
        }
        rp_hash(P, in, r, o1);
        for (size_t i = 0; i < r; i++){
            in[i] += p;
        }
        rp_hash(P, in, r, o2);
        CHECK(eq(o1, o2, r));
    }

    for (size_t i = 0; i < 2*r; i++){
        in[i] = rnd_fe(p);
    }
    rp_hash(P, in, 2*r, o1);
    in[2*r - 1] = fp_add(in[2*r - 1], 1, p);
    rp_hash(P, in, 2*r, o2);
    CHECK(!eq(o1, o2, r));
}

int main(int argc, char** argv){
    const char* arq = argc > 1 ? argv[1] : "../ref/vectors.txt";
    vf = fopen(arq, "r");
    if (!vf){
        perror(arq);
        return 2;
    }
    printf("vetores: %s\n", arq);

    rp_params P = {0};
    int tem = 0;
    int cont[6] = {0}; 
    int cont_falha[6] = {0};
    char tag[16];
    fe_t a[RP_MAX_M], b[RP_MAX_M];

    while (fscanf(vf, "%15s", tag) == 1){
        int f0 = falhas;
        int tipo = -1;
        if (!strcmp(tag, "PARAMS")){
            if (tem){
                printf("  Parte B (propriedades)\n");
                propriedades(&P);
                rp_params_free(&P);
            }
            uint64_t p = rd(), m = rd(), c = rd(), s = rd();
            if (rp_params_init(&P, p, m, c, s)){
                printf("rp_params_init falhou\n");
                return 1;
            }
            tem = 1;
            printf("p=%" PRIu64 " m=%u c=%u s=%u (alpha=%" PRIu64 ", N=%u)\n",
                   P.p, P.m, P.c, P.s, P.alpha, P.N);
            printf("  Parte A (vetores de referencia)\n");
            continue;
        }
        else if (!strcmp(tag, "META")){
            tipo = 0;
            uint64_t al = rd(), ai = rd(), N = rd();
            CHECK(al == P.alpha && ai == P.alphainv && N == P.N);
            if (N != P.N){ 
                printf("N diferente, abortando\n");
                return 1;
            }
        }
        else if (!strcmp(tag, "MDS")){
            tipo = 1;
            unsigned m = P.m, ruins = 0;
            for (unsigned i = 0; i < m*m; i++){
                ruins += (rd() != P.mds[i]);
            }
            CHECK(ruins == 0);
        }
        else if (!strcmp(tag, "RC")){
            tipo = 2;
            unsigned cnt = 2 * P.m * P.N, ruins = 0;
            for (unsigned i = 0; i < cnt; i++){
                ruins += (rd() != P.rc[i]);
            }
            CHECK(ruins == 0);
        }
        else if (!strcmp(tag, "TRACE")){
            tipo = 3;
            fe_t tr[7 * RP_MAX_M];
            rd_vec(tr, 7 * P.m);
            rp_params Q = P; 
            Q.N = 1;
            memcpy(a, tr, P.m * sizeof(fe_t));
            rp_permutation(&Q, a);
            CHECK(eq(a, tr + 6 * P.m, P.m));
            if (falhas != f0){
                diagnostico_trace(&P, tr);
            }
        }
        else if (!strcmp(tag, "PERM")){
            tipo = 4;
            rd_vec(a, P.m);
            rd_vec(b, P.m);
            rp_permutation(&P, a);
            CHECK(eq(a, b, P.m));
        }
        else if (!strcmp(tag, "HASH")){
            tipo = 5;
            size_t n = rd();
            fe_t* in = malloc(sizeof(fe_t) * (n + 1)); 
            fe_t out[RP_MAX_M];
            rd_vec(in, n);
            rd_vec(b, P.rate);
            rp_hash(&P, in, n, out);
            CHECK(eq(out, b, P.rate));
            if (falhas != f0){
                printf("    (hash com n=%zu)\n", n);
            }
            free(in);
        }
        else{
            fprintf(stderr, "tag desconhecida: %s\n", tag);
            return 2;
        }
        cont[tipo]++;
        cont_falha[tipo] += (falhas != f0);
    }
    if (tem){
        printf("  Parte B (propriedades)\n");
        propriedades(&P);
        rp_params_free(&P);
    }
    fclose(vf);

    const char* nomes[6] = {"META", "MDS", "RC", "TRACE", "PERM", "HASH"};
    printf("\nresumo dos vetores de referencia:\n");
    for (int k = 0; k < 6; k++){
        printf("  %-5s %3d/%-3d ok\n", nomes[k], cont[k] - cont_falha[k], cont[k]);
    }
    if (falhas == 0){
        printf("todos os %d testes passaram\n", total);
    }
    else{
        printf("%d de %d testes falharam\n", falhas, total);
    }
    return falhas != 0;
}
