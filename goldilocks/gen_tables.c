// gen_tables.c : usa a versao generica pra escrever o gl_tables.h com a MDS e as constantes
// uso: ./gen_tables > gl_tables.h
#include <inttypes.h>
#include <stdio.h>
#include "rescue.h"
#include "gl.h"

int main(void){
    rp_params P = {0};
    if (rp_params_init(&P, GL_P, GL_M, GL_M - GL_RATE, 128)){
        fprintf(stderr, "rp_params_init falhou\n");
        return 1;
    }

    printf("// gerado pelo gen_tables.c, nao editar na mao\n");
    printf("#define GL_N %u\n\n", P.N);

    printf("static const uint64_t GL_MDS[%d][%d] = {\n", GL_M, GL_M);
    for (int i = 0; i < GL_M; i++){
        printf("    {");
        for (int j = 0; j < GL_M; j++){
            printf("%" PRIu64 "ULL%s", P.mds[i*GL_M + j], j < GL_M - 1 ? ", " : "");
        }
        printf("},\n");
    }
    printf("};\n\n");

    printf("static const uint64_t GL_RC[%u][%d] = {\n", 2*P.N, GL_M);
    for (unsigned k = 0; k < 2*P.N; k++){
        printf("    {");
        for (int j = 0; j < GL_M; j++){
            printf("%" PRIu64 "ULL%s", P.rc[k*GL_M + j], j < GL_M - 1 ? ", " : "");
        }
        printf("},\n");
    }
    printf("};\n");

    rp_params_free(&P);
    return 0;
}
