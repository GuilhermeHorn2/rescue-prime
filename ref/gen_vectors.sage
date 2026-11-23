# gen_vectors.sage : exporta os vetores de teste do codigo de referencia
# (KULeuven-COSIC/Marvellous, rescue_prime.sage, sem modificar nada)
#
# uso (dentro dessa pasta):
#     sage gen_vectors.sage            -> escreve vectors.txt
#
# formato : um registro por linha, todos os numeros em decimal
#   PARAMS p m c s
#   META   alpha alphainv N
#   MDS    a_00 a_01 ... a_(m-1)(m-1)          (linha por linha, m*m valores)
#   RC     k_0 ... k_(2mN-1)                   (todas as constantes de round)
#   TRACE  x | s1 | s2 | s3 | s4 | s5 | s6     (round 0, depois de cada um dos 6 passos)
#   PERM   x_0..x_(m-1) y_0..y_(m-1)           (y = permutation(x))
#   HASH   n x_1..x_n y_0..y_(r-1)             (y = hash(x))
load("marvellous/rescue_prime.sage")
import random # rng do python, pra dar as mesmas entradas do gen_vectors.py
random.seed(int(2026))

param_sets = [
    (2^64 - 2^32 + 1, 12, 4, 128),   # Goldilocks
    (2^31 - 1,        12, 4, 128),   # Mersenne-31
    (2^61 - 1,         8, 4, 128),   # Mersenne-61
    (65537,            4, 2,  64),
    (101,              3, 1,  16),   # pequeno, da pra debugar na mao
    (2^64 - 59,        5, 2, 128),   # maior primo de 64 bits
]

def col(x):
    return matrix([[xi] for xi in x])

def line(tag, values):
    return tag + " " + " ".join(str(v) for v in values) + "\n"

with open("vectors.txt", "w") as f:
    for (p, m, c, s) in param_sets:
        params = rescue_prime_parameters(p, m, c, s)
        _, _, _, _, alpha, alphainv, N, MDS, rc = params
        Fp = FiniteField(p)
        r = m - c
        rnd = lambda: Fp(random.randrange(int(p)))

        f.write("PARAMS %d %d %d %d\n" % (p, m, c, s))
        f.write("META %d %d %d\n" % (alpha, alphainv, N))
        f.write(line("MDS", MDS.list()))
        f.write(line("RC", rc))

        # TRACE feito passo a passo com a MDS e as constantes da referencia
        x = [rnd() for _ in range(m)]
        st = col(x)
        trace = list(x)
        for j in range(m): st[j,0] = st[j,0]^alpha
        trace += st.list()
        st = MDS * st;                                   trace += st.list()
        for j in range(m): st[j,0] += rc[j]
        trace += st.list()
        for j in range(m): st[j,0] = st[j,0]^alphainv
        trace += st.list()
        st = MDS * st;                                   trace += st.list()
        for j in range(m): st[j,0] += rc[m + j]
        trace += st.list()
        f.write(line("TRACE", trace))

        states = [[Fp(0)] * m, [Fp(p - 1)] * m, [Fp(1)] + [Fp(0)] * (m - 1)]
        states += [[rnd() for _ in range(m)] for _ in range(3)]
        for x in states:
            y = rescue_XLIX_permutation(params, copy(col(x)))   # copy pq a funcao muda o argumento
            f.write(line("PERM", x + y.list()))

        inputs = [[rnd() for _ in range(n)] for n in sorted(set([0, 1, r - 1, r, r + 1, 2*r, 3*r + 2]))]
        inputs.append([Fp(0)] * (r - 1))     # zeros logo antes do 1 do padding
        inputs.append([Fp(p - 1)] * r)       # maior elemento, bloco cheio
        for x in inputs:
            y = rescue_prime_wrapper(params, x)
            f.write("HASH %d %s\n" % (len(x), " ".join(str(v) for v in x + y)))
