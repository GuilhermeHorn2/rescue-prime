CC     = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -I../src -I.
LIBS   = -lcrypto -lm

# a versao generica so serve pra gerar as tabelas e de referencia nos testes
CORE = ../src/params.c ../src/fp.c
VECTORS ?= $(firstword $(wildcard ../ref/vectors.txt) ../ref/vectors_py.txt)

all: test_gl

gen_tables: gen_tables.c gl.h $(CORE)
	$(CC) $(CFLAGS) -o $@ gen_tables.c $(CORE) $(LIBS)

gl_tables.h: gen_tables
	./gen_tables > $@

# COUNT_OPS liga o contador de mults (confere os 72 do x^(7^-1))
test_gl: test_gl.c gl.c gl.h gl_tables.h ../src/rescue.c $(CORE)
	$(CC) $(CFLAGS) -DCOUNT_OPS -o $@ test_gl.c ../src/rescue.c $(CORE) $(LIBS)

test: test_gl
	./test_gl $(VECTORS)

asan: gl_tables.h
	$(CC) -std=c11 -O1 -g -Wall -Wextra -I../src -I. -DCOUNT_OPS -fsanitize=address,undefined -fno-omit-frame-pointer \
	    -o test_gl_asan test_gl.c ../src/rescue.c $(CORE) $(LIBS)
	./test_gl_asan $(VECTORS)

# testa tambem cada otimizacao desligada
variantes: gl_tables.h
	@for f in -DGL_SEM_REDUCAO -DGL_SEM_CADEIA -DGL_SEM_MDS_LAZY; do \
	    $(CC) $(CFLAGS) -DCOUNT_OPS $$f -o test_gl_var test_gl.c ../src/rescue.c $(CORE) $(LIBS) || exit 1; \
	    printf "%-18s " "$$f"; ./test_gl_var $(VECTORS) | tail -1; \
	done; rm -f test_gl_var

clean:
	rm -f gen_tables gl_tables.h test_gl test_gl_asan test_gl_var

.PHONY: all test asan variantes clean
