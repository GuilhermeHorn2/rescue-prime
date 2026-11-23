// perfctr.h : ciclos, instrucoes e branch-misses (so user space) via perf_event_open
// precisa de kernel.perf_event_paranoid <= 2
#ifndef PERFCTR_H
#define PERFCTR_H

#include <linux/perf_event.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#define PC_N 3 // ciclos, instrucoes, branch-misses

static int pc_fd[PC_N] = {-1, -1, -1};
static int pc_ok = 0;

static int pc_abre(uint64_t config, int lider){
    struct perf_event_attr a;
    memset(&a, 0, sizeof a);
    a.type = PERF_TYPE_HARDWARE;
    a.size = sizeof a;
    a.config = config;
    a.disabled = lider == -1;
    a.exclude_kernel = 1;
    a.exclude_hv = 1;
    a.read_format = PERF_FORMAT_GROUP;
    return (int)syscall(SYS_perf_event_open, &a, 0, -1, lider, 0);
}

static void pc_init(void){
    pc_fd[0] = pc_abre(PERF_COUNT_HW_CPU_CYCLES, -1);
    if (pc_fd[0] < 0){
        fprintf(stderr, "perf_event_open falhou (perf_event_paranoid?), so tempo\n");
        return;
    }
    pc_fd[1] = pc_abre(PERF_COUNT_HW_INSTRUCTIONS, pc_fd[0]);
    pc_fd[2] = pc_abre(PERF_COUNT_HW_BRANCH_MISSES, pc_fd[0]);
    pc_ok = pc_fd[1] >= 0 && pc_fd[2] >= 0;
}

static void pc_start(void){
    if (pc_ok){
        ioctl(pc_fd[0], PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
        ioctl(pc_fd[0], PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
    }
}

static void pc_stop(uint64_t v[PC_N]){
    struct { uint64_t nr; uint64_t val[PC_N]; } r = {0, {0}};
    if (pc_ok){
        ioctl(pc_fd[0], PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
        if (read(pc_fd[0], &r, sizeof r) != (ssize_t)sizeof r){
            memset(&r, 0, sizeof r);
        }
    }
    memcpy(v, r.val, sizeof r.val);
}

static double agora_ns(void){
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e9 + t.tv_nsec;
}

// mediana de cada metrica, por iteracao
typedef struct {
    double ns, cic, ins, bm;
} Medida;

#define REPS 31

static int cmp_double(const void* a, const void* b){
    double x = *(const double*)a;
    double y = *(const double*)b;
    return (x > y) - (x < y);
}

static double mediana(double* v){
    qsort(v, REPS, sizeof(double), cmp_double);
    return v[REPS/2];
}

// roda f(n) 1 vez de aquecimento + REPS medidas
static Medida mede(void (*f)(int), int n){
    double ns[REPS], cic[REPS], ins[REPS], bm[REPS];
    uint64_t v[PC_N];
    f(n);
    for (int k = 0; k < REPS; k++){
        pc_start();
        double t0 = agora_ns();
        f(n);
        double t1 = agora_ns();
        pc_stop(v);
        ns[k] = (t1 - t0) / n;
        cic[k] = (double)v[0] / n;
        ins[k] = (double)v[1] / n;
        bm[k] = (double)v[2] / n;
    }
    Medida m = { mediana(ns), mediana(cic), mediana(ins), mediana(bm) };
    return m;
}

#endif
