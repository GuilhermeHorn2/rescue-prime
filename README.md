# Rescue-Prime em C
#
Implementação em C da função de hash Rescue-Prime ([ePrint 2020/1143](https://eprint.iacr.org/2020/1143)) para primos de até 64 bits, com geração completa dos parâmetros a partir de `(p, m, c, s)`, e uma versão especializada para o corpo Goldilocks (`p = 2^64 - 2^32 + 1`, `m = 12`, `c = 4`).

## Estrutura

| Diretório | Conteúdo |
|---|---|
| `src/` | Versão genérica: aritmética em F_p (`fp.h`, `fp.c`), geração de parâmetros (`params.c`), permutação e hash (`rescue.c`, `rescue.h`), testes (`test_rescue.c`) e teste de mutação (`mutantes.sh`) |
| `goldilocks/` | Versão especializada (`gl.c`, `gl.h`), gerador das tabelas (`gen_tables.c`) e testes contra a versão genérica e o Sage (`test_gl.c`) |
| `bench/` | Medidas: contagem de multiplicações (`custo.c`), perfil genérico x Goldilocks (`prof.c`), ablação (alvo `ablacao` do `Makefile`) e comparação com o SHA3-256 (`sha3.c`). `perfctr.h` lê os contadores de hardware |
| `ref/` | Scripts que exportam os vetores de teste do código de referência (`gen_vectors.sage`), um porte em Python (`ref.py`, `gen_vectors.py`) e os vetores gerados |


## Requisitos

- GCC (testado com 14.3) e GNU Make
- OpenSSL com cabeçalhos de desenvolvimento (SHAKE-256 e SHA3-256)
- Python 3, para o porte da referência
- Opcional: SageMath, para regenerar os vetores a partir do código dos autores
- Para as medidas: Linux com `kernel.perf_event_paranoid <= 2`

No Debian/Ubuntu:

```sh
sudo apt install build-essential libssl-dev python3
```

## Início rápido

```sh
git clone <url-do-repo> && cd rescue-prime
make test
```

Os binários não são versionados; o `make` compila tudo na primeira execução.

## Uso

```sh
make test        # 176 testes da versão genérica + 13 da Goldilocks
make asan        # os mesmos testes com AddressSanitizer e UBSan
make mutantes    # teste de mutação do rescue.c
make variantes   # testes da Goldilocks com cada otimização desligada
make bench       # todas as medidas de desempenho
```

As medidas fixam o processo no núcleo 2 (`CPU=2` no `bench/Makefile`). Se os contadores de hardware estiverem bloqueados:

```sh
sudo sysctl kernel.perf_event_paranoid=1
```

## Vetores de teste

`ref/vectors.txt` foi gerado pelo código de referência dos autores e é idêntico byte a byte a `ref/vectors_py.txt`, gerado pelo porte em Python. O código dos autores ([KULeuven-COSIC/Marvellous](https://github.com/KULeuven-COSIC/Marvellous)) já está em `ref/marvellous`. Para regenerar os vetores:

```sh
make vetores-sage   # precisa do SageMath
make vetores-py     # só Python 3
```



