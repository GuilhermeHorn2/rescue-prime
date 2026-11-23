# gen_vectors.py : mesma coisa do gen_vectors.sage so que usando o ref.py, sem Sage
# uso: python3 gen_vectors.py vectors_py.txt
import random, sys
from ref import params, perm, hash_

random.seed(2026)
param_sets = [(2**64 - 2**32 + 1, 12, 4, 128), (2**31 - 1, 12, 4, 128),
              (2**61 - 1, 8, 4, 128), (65537, 4, 2, 64), (101, 3, 1, 16),
              (2**64 - 59, 5, 2, 128)]

def line(tag, values):
    return tag + " " + " ".join(str(v) for v in values) + "\n"

with open(sys.argv[1] if len(sys.argv) > 1 else "vectors_py.txt", "w") as f:
    for (p, m, c, s) in param_sets:
        P = params(p, m, c, s); r = m - c
        M, rc, a, ai = P["MDS"], P["rc"], P["alpha"], P["alphainv"]
        rnd = lambda: random.randrange(p)
        mds = lambda st: [sum(M[i][k] * st[k] for k in range(m)) % p for i in range(m)]
        f.write("PARAMS %d %d %d %d\n" % (p, m, c, s))
        f.write("META %d %d %d\n" % (a, ai, P["N"]))
        f.write(line("MDS", [M[i][j] for i in range(m) for j in range(m)]))
        f.write(line("RC", rc))
        x = [rnd() for _ in range(m)]
        s1 = [pow(v, a, p) for v in x]; s2 = mds(s1)
        s3 = [(s2[j] + rc[j]) % p for j in range(m)]
        s4 = [pow(v, ai, p) for v in s3]; s5 = mds(s4)
        s6 = [(s5[j] + rc[m + j]) % p for j in range(m)]
        f.write(line("TRACE", x + s1 + s2 + s3 + s4 + s5 + s6))
        states = [[0] * m, [p - 1] * m, [1] + [0] * (m - 1)]
        states += [[rnd() for _ in range(m)] for _ in range(3)]
        for st in states:
            f.write(line("PERM", st + perm(P, st)))
        inputs = [[rnd() for _ in range(n)] for n in sorted({0, 1, r - 1, r, r + 1, 2 * r, 3 * r + 2})]
        inputs.append([0] * (r - 1))
        inputs.append([p - 1] * r)
        for x in inputs:
            f.write("HASH %d %s\n" % (len(x), " ".join(map(str, x + hash_(P, x)))))
