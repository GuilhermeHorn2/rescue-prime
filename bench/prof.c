// prof.c : perfil do generico x goldilocks, total e por operacao
// inclui o rescue.c e o gl.c direto pra medir as funcoes static
// uso: taskset -c 2 ./prof      -> tabelas
//      taskset -c 2 ./prof -p   -> so a gl_permutation (make ablacao)
#define _GNU_SOURCE
#include <stdlib.h>
#include "perfctr.h"
#include "../src/rescue.c"
#include "../goldilocks/gl.c"

static rp_params P;
static uint64_t st[GL_M];
static volatile uint64_t lixo; // pro compilador nao jogar as contas fora

static void reinicia(void){
    for (int i = 0; i < GL_M; i++){
        st[i] = 0x9E3779B97F4A7C15ULL * (i + 1) % GL_P;
    }
}

// operacoes num elemento, cada iteracao depende da anterior (latencia)
static void mul_gen(int n){
    uint64_t x = st[0];
    for (int k = 0; k < n; k++){
        x = fp_mul(x, st[1], P.p);
    }
    lixo += x;
}

static void mul_gl(int n){
    uint64_t x = st[0];
    for (int k = 0; k < n; k++){
        x = gl_mul(x, st[1]);
    }
    lixo += x;
}

static void pow7_gen(int n){
    uint64_t x = st[0];
    for (int k = 0; k < n; k++){
        x = fp_pow(x, P.alpha, P.p);
    }
    lixo += x;
}

static void pow7_gl(int n){
    uint64_t x = st[0];
    for (int k = 0; k < n; k++){
        x = gl_pow7(x);
    }
    lixo += x;
}

static void powinv_gen(int n){
    uint64_t x = st[0];
    for (int k = 0; k < n; k++){
        x = fp_pow(x, P.alphainv, P.p);
    }
    lixo += x;
}

static void powinv_gl(int n){
    uint64_t x = st[0];
    for (int k = 0; k < n; k++){
        x = gl_sbox_inv(x);
    }
    lixo += x;
}

// passos da rodada no estado inteiro
static void s_gen(int n){
    for (int k = 0; k < n; k++){
        sbox(&P, st, P.alpha);
    }
    lixo += st[0];
}

static void s_gl(int n){
    for (int k = 0; k < n; k++){
        for (int j = 0; j < GL_M; j++){
            st[j] = gl_pow7(st[j]);
        }
    }
    lixo += st[0];
}

static void sinv_gen(int n){
    for (int k = 0; k < n; k++){
        sbox(&P, st, P.alphainv);
    }
    lixo += st[0];
}

static void sinv_gl(int n){
    for (int k = 0; k < n; k++){
        for (int j = 0; j < GL_M; j++){
            st[j] = gl_sbox_inv(st[j]);
        }
    }
    lixo += st[0];
}

static void mds_gen(int n){
    for (int k = 0; k < n; k++){
        mds_mul(&P, st);
    }
    lixo += st[0];
}

static void mds_gl(int n){
    for (int k = 0; k < n; k++){
        gl_mds(st);
    }
    lixo += st[0];
}

static void ark_gen(int n){
    for (int k = 0; k < n; k++){
        add_constants(&P, st, P.rc);
    }
    lixo += st[0];
}

static void ark_gl(int n){
    for (int k = 0; k < n; k++){
        for (int j = 0; j < GL_M; j++){
            st[j] = fp_add(st[j], GL_RC[0][j], GL_P);
        }
    }
    lixo += st[0];
}

static void perm_gen(int n){
    for (int k = 0; k < n; k++){
        rp_permutation(&P, st);
    }
    lixo += st[0];
}

static void perm_gl(int n){
    for (int k = 0; k < n; k++){
        gl_permutation(st);
    }
    lixo += st[0];
}

typedef struct {
    Medida g, l;
} Par;

static Par par(void (*fg)(int), void (*fl)(int), int n){
    Par r;
    reinicia();
    r.g = mede(fg, n);
    reinicia();
    r.l = mede(fl, n);
    return r;
}

int main(int argc, char** argv){
    if (rp_params_init(&P, GL_P, GL_M, GL_M - GL_RATE, 128)){
        printf("rp_params_init falhou\n");
        return 1;
    }
    pc_init();

    if (argc > 1 && !strcmp(argv[1], "-p")){
        reinicia();
        Medida l = mede(perm_gl, 2000);
        printf("%.0f %.0f %.0f %.0f\n", l.cic, l.ns, l.ins, l.bm);
        rp_params_free(&P);
        return 0;
    }

    printf("operacoes num elemento (ciclos por operacao)\n");
    printf("%-10s %10s %10s %7s\n", "", "generico", "goldilocks", "razao");
    const char* nome_op[3] = {"mul", "x^7", "x^(7^-1)"};
    Par op[3] = {
        par(mul_gen, mul_gl, 1000000),
        par(pow7_gen, pow7_gl, 200000),
        par(powinv_gen, powinv_gl, 20000),
    };
    for (int i = 0; i < 3; i++){
        printf("%-10s %10.1f %10.1f %6.2fx\n", nome_op[i], op[i].g.cic, op[i].l.cic, op[i].g.cic / op[i].l.cic);
    }

    // vezes por permutacao: N para as sboxes, 2N pra MDS e constantes
    const char* nome[4] = {"S", "S^-1", "MDS", "constantes"};
    int vezes[4] = {GL_N, GL_N, 2*GL_N, 2*GL_N};
    Par ps[4] = {
        par(s_gen, s_gl, 20000),
        par(sinv_gen, sinv_gl, 2000),
        par(mds_gen, mds_gl, 20000),
        par(ark_gen, ark_gl, 200000),
    };
    double sg = 0, sl = 0;
    for (int i = 0; i < 4; i++){
        sg += ps[i].g.cic * vezes[i];
        sl += ps[i].l.cic * vezes[i];
    }

    printf("\npassos no estado inteiro (ciclos por chamada)\n");
    printf("%-10s %10s %10s %7s\n", "", "generico", "goldilocks", "razao");
    for (int i = 0; i < 4; i++){
        printf("%-10s %10.0f %10.0f %6.2fx\n", nome[i], ps[i].g.cic, ps[i].l.cic, ps[i].g.cic / ps[i].l.cic);
    }

    Par pp = par(perm_gen, perm_gl, 2000);
    printf("\npassos numa permutacao (kciclos e fracao da soma)\n");
    printf("%-10s %10s %10s %8s %8s\n", "", "generico", "goldilocks", "%gen", "%gl");
    for (int i = 0; i < 4; i++){
        double g = ps[i].g.cic * vezes[i], l = ps[i].l.cic * vezes[i];
        printf("%-10s %10.1f %10.1f %7.1f%% %7.1f%%\n", nome[i], g / 1e3, l / 1e3, 100 * g / sg, 100 * l / sl);
    }
    printf("%-10s %10.1f %10.1f\n", "soma", sg / 1e3, sl / 1e3);
    printf("%-10s %10.1f %10.1f   (soma/medido: %.1f%% %.1f%%)\n", "medido", pp.g.cic / 1e3, pp.l.cic / 1e3,
           100 * sg / pp.g.cic, 100 * sl / pp.l.cic);

    printf("\npermutacao completa\n");
    printf("%-14s %12s %12s %7s\n", "", "generico", "goldilocks", "razao");
    printf("%-14s %12.0f %12.0f %6.2fx\n", "ciclos", pp.g.cic, pp.l.cic, pp.g.cic / pp.l.cic);
    printf("%-14s %12.0f %12.0f %6.2fx\n", "ns", pp.g.ns, pp.l.ns, pp.g.ns / pp.l.ns);
    printf("%-14s %12.0f %12.0f %6.2fx\n", "instrucoes", pp.g.ins, pp.l.ins, pp.g.ins / pp.l.ins);
    printf("%-14s %12.2f %12.2f\n", "IPC", pp.g.ins / pp.g.cic, pp.l.ins / pp.l.cic);
    printf("%-14s %12.0f %12.0f\n", "branch-misses", pp.g.bm, pp.l.bm);

    rp_params_free(&P);
    return lixo == 42;
}
