# Makefile da raiz : atalhos pros Makefiles de cada diretorio

VECTORS ?= $(firstword $(wildcard $(CURDIR)/ref/vectors.txt) $(CURDIR)/ref/vectors_py.txt)

all: test

# testes da versao generica e da Goldilocks
test:
	$(MAKE) -C src test VECTORS=$(VECTORS)
	$(MAKE) -C goldilocks test VECTORS=$(VECTORS)

# os mesmos testes com AddressSanitizer + UBSan
asan:
	$(MAKE) -C src asan VECTORS=$(VECTORS)
	$(MAKE) -C goldilocks asan VECTORS=$(VECTORS)

# teste de mutacao do rescue.c (tabela de mutantes)
mutantes:
	cd src && ./mutantes.sh $(VECTORS)

# testes do Goldilocks com cada otimizacao desligada
variantes:
	$(MAKE) -C goldilocks variantes VECTORS=$(VECTORS)

# todas as medidas do relatorio (precisa de kernel.perf_event_paranoid <= 2)
bench:
	$(MAKE) -C bench run-custo run-prof ablacao run-sha3

# vetores de teste: com o Sage (precisa do ref/marvellous, ver README) ou com o porte em Python
vetores-sage:
	cd ref && PYTHONPATH=$(CURDIR)/ref/marvellous sage gen_vectors.sage

vetores-py:
	cd ref && python3 gen_vectors.py vectors_py.txt

relatorio:
	cd relatorio && lualatex relatorio && bibtex relatorio && lualatex relatorio && lualatex relatorio

clean:
	$(MAKE) -C src clean
	$(MAKE) -C goldilocks clean
	$(MAKE) -C bench clean

.PHONY: all test asan mutantes variantes bench vetores-sage vetores-py relatorio clean
