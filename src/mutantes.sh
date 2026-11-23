#!/bin/sh
# mutantes.sh : coloca uns bugs de proposito no rescue.c e ve se o test_rescue pega
# uso: ./mutantes.sh [arquivo de vetores]
VEC=${1:-../ref/vectors_py.txt}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

# $1 = nome do mutante, $2 = expressao do sed que coloca o bug
mutar(){
    sed "$2" rescue.c > "$TMP/m.c"
    if cmp -s rescue.c "$TMP/m.c"; then
        echo "  [ERRO] mutante '$1' nao alterou nada"
        return
    fi
    gcc -std=c11 -O2 -w -I. -o "$TMP/t" test_rescue.c "$TMP/m.c" params.c fp.c -lcrypto -lm || return
    out=$("$TMP/t" "$VEC" 2>&1)
    st=$?
    if [ $st -eq 0 ]; then
        echo "  SOBREVIVEU  $1"
    else
        resumo=$(echo "$out" | grep -E '^ +(META|MDS|RC|TRACE|PERM|HASH) ' \
                 | awk '$2 !~ /^([0-9]+)\/\1$/ {printf "%s %s  ", $1, $2}')
        echo "  morto       $1   ($resumo)"
    fi
}

echo "mutantes do rescue.c:"
mutar "sbox inversa usa alpha"          's/sbox(P, st, P->alphainv)/sbox(P, st, P->alpha)/'
mutar "constantes K_2i+1 = K_2i"        's/P->rc + 2\*m\*i + m)/P->rc + 2*m*i)/'
mutar "MDS transposta"                  's/P->mds\[i\* P->m + j\]/P->mds[j* P->m + i]/'
mutar "absorcao sobrescreve (=)"        's/st\[i\] = fp_add(st\[i\], x, P->p)/st[i] = x/'
mutar "padding sem o 1"                 's/(idx == n ? 1 : 0)/0/'
mutar "padding 1 no inicio do bloco"    's/(idx == n ? 1 : 0)/(idx == (n\/r)*r ? 1 : 0)/'
mutar "nao reduz a entrada (equivalente)"       's/in\[idx\] % P->p/in[idx]/'
mutar "um round a menos"                's/i < P->N; i++/i + 1 < P->N; i++/'
mutar "saida = capacidade"              's/memcpy(out, st, /memcpy(out, st + P->c, /'
mutar "ordem: constantes antes da MDS"  '0,/mds_mul(P, st);/{/mds_mul(P, st);/d}; s/add_constants(P, st, P->rc + 2\*m\*i);/add_constants(P, st, P->rc + 2*m*i); mds_mul(P, st);/'
