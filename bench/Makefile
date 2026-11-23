CC     = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -I../src -I../goldilocks
LIBS   = -lcrypto -lm
CORE   = ../src/params.c ../src/fp.c
CPU   ?= 2

all: prof custo sha3

../goldilocks/gl_tables.h:
	$(MAKE) -C ../goldilocks gl_tables.h

prof: prof.c perfctr.h ../src/rescue.c ../goldilocks/gl.c ../goldilocks/gl_tables.h $(CORE)
	$(CC) $(CFLAGS) -o $@ prof.c $(CORE) $(LIBS)

custo: custo.c ../src/rescue.c ../goldilocks/gl.c ../goldilocks/gl_tables.h $(CORE)
	$(CC) $(CFLAGS) -DCOUNT_OPS -o $@ custo.c ../src/rescue.c $(CORE) $(LIBS)

sha3: sha3.c perfctr.h ../src/rescue.c ../goldilocks/gl.c ../goldilocks/gl_tables.h $(CORE)
	$(CC) $(CFLAGS) -o $@ sha3.c ../src/rescue.c ../goldilocks/gl.c $(CORE) $(LIBS)

# tabela de custo em multiplicacoes
run-custo: custo
	./custo

# perfil generico x goldilocks
run-prof: prof
	taskset -c $(CPU) ./prof

# comparacao com o SHA3-256
run-sha3: sha3
	taskset -c $(CPU) ./sha3

# liga as otimizacoes uma a uma e depois tira uma de cada vez
# saida: ciclos ns instrucoes branch-misses por permutacao
ABL = "nenhuma|-DGL_SEM_REDUCAO -DGL_SEM_CADEIA -DGL_SEM_MDS_LAZY" \
      "+ reducao|-DGL_SEM_CADEIA -DGL_SEM_MDS_LAZY" \
      "+ reducao + cadeia|-DGL_SEM_MDS_LAZY" \
      "todas|" \
      "todas - reducao|-DGL_SEM_REDUCAO" \
      "todas - cadeia|-DGL_SEM_CADEIA" \
      "todas - mds lazy|-DGL_SEM_MDS_LAZY"

ablacao: ../goldilocks/gl_tables.h
	@printf "%-20s %9s %9s %9s %9s\n" "" ciclos ns instr br-miss
	@for v in $(ABL); do \
	    nome=$${v%%|*}; flags=$${v#*|}; \
	    $(CC) $(CFLAGS) $$flags -o prof_abl prof.c $(CORE) $(LIBS) || exit 1; \
	    printf "%-20s %9s %9s %9s %9s\n" "$$nome" $$(taskset -c $(CPU) ./prof_abl -p); \
	done; rm -f prof_abl

clean:
	rm -f prof custo sha3 prof_abl

.PHONY: all run-custo run-prof run-sha3 ablacao clean
