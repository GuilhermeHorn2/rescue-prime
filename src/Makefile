CC     = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra
LIBS   = -lcrypto -lm

# usa os vetores do Sage se existirem, senao os do port em Python
VECTORS ?= $(firstword $(wildcard ../ref/vectors.txt) ../ref/vectors_py.txt)

all: fp params test_rescue

fp: fp.c fp.h
	$(CC) $(CFLAGS) -DFP_TEST -o $@ fp.c

params: params.c fp.c fp.h rescue.h
	$(CC) $(CFLAGS) -DPARAMS_TEST -o $@ params.c fp.c $(LIBS)

test_rescue: test_rescue.c rescue.c params.c fp.c fp.h rescue.h
	$(CC) $(CFLAGS) -o $@ test_rescue.c rescue.c params.c fp.c $(LIBS)

# AddressSanitizer + UBSan
test_rescue_asan: test_rescue.c rescue.c params.c fp.c fp.h rescue.h
	$(CC) -std=c11 -O1 -g -Wall -Wextra -fsanitize=address,undefined -fno-omit-frame-pointer \
	    -o $@ test_rescue.c rescue.c params.c fp.c $(LIBS)

test: fp params test_rescue
	./fp
	./params
	./test_rescue $(VECTORS)

asan: test_rescue_asan
	./test_rescue_asan $(VECTORS)

clean:
	rm -f fp params test_rescue test_rescue_asan

.PHONY: all test asan clean
