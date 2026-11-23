# ref.py : port pra python do rescue_prime.sage, pra gerar os vetores sem precisar do Sage
import hashlib, math, sys, random
from math import comb, gcd, ceil, floor

def factor(n):
    fs = set()
    d = 2
    while d * d <= n and d < 10**6:
        while n % d == 0:
            fs.add(d); n //= d
        d += 1
    if n > 1:
        stack = [n]
        while stack:
            x = stack.pop()
            if is_prime(x): fs.add(x); continue
            f = rho(x); stack += [f, x // f]
    return fs

def is_prime(n):
    if n < 2: return False
    for a in [2,3,5,7,11,13,17,19,23,29,31,37]:
        if n % a == 0: return n == a
    d, s = n-1, 0
    while d % 2 == 0: d//=2; s+=1
    for a in [2,3,5,7,11,13,17,19,23,29,31,37]:
        x = pow(a,d,n)
        if x in (1,n-1): continue
        for _ in range(s-1):
            x = x*x % n
            if x == n-1: break
        else: return False
    return True

def rho(n):
    if n % 2 == 0: return 2
    rng = random.Random(n) # rng separado pra nao mexer no global que gera as entradas
    while True:
        c = rng.randrange(1, n); x = y = rng.randrange(2, n); d = 1
        while d == 1:
            x = (x*x+c) % n; y = (y*y+c) % n; y = (y*y+c) % n
            d = gcd(abs(x-y), n)
        if d != n: return d

def get_alphas(p):
    for alpha in range(3, p):
        if gcd(alpha, p-1) == 1: break
    return alpha, pow(alpha, -1, p-1)

def get_number_of_rounds(p, m, c, s, alpha):
    rate = m - c
    dcon = lambda N: floor(0.5*(alpha-1)*m*(N-1) + 2)
    v = lambda N: m*(N-1) + rate
    target = 2**s
    for l1 in range(1, 25):
        if comb(v(l1)+dcon(l1), v(l1))**2 > target: break
    return ceil(1.5*max(5, l1))

def get_mds_matrix(p, m):
    fs = factor(p-1)
    g = 2
    while any(pow(g, (p-1)//q, p) == 1 for q in fs): g += 1
    V = [[pow(g, i*j, p) for j in range(2*m)] for i in range(m)]
    r = 0
    for col in range(2*m):
        piv = next((i for i in range(r, m) if V[i][col] % p), None)
        if piv is None: continue
        V[r], V[piv] = V[piv], V[r]
        inv = pow(V[r][col], -1, p)
        V[r] = [x*inv % p for x in V[r]]
        for i in range(m):
            if i != r and V[i][col]:
                f = V[i][col]
                V[i] = [(a - f*b) % p for a, b in zip(V[i], V[r])]
        r += 1
        if r == m: break
    right = [row[m:] for row in V]
    return [[right[j][i] for j in range(m)] for i in range(m)], g

def get_round_constants(p, m, c, s, N):
    bpi = ceil(len(bin(p)[2:]) / 8) + 1
    nb = bpi * 2 * m * N
    seed = "Rescue-XLIX(%i,%i,%i,%i)" % (p, m, c, s)
    bs = hashlib.shake_256(seed.encode("ascii")).digest(nb)
    return [int.from_bytes(bs[bpi*i:bpi*(i+1)], "little") % p for i in range(2*m*N)]

def params(p, m, c, s):
    a, ai = get_alphas(p)
    N = get_number_of_rounds(p, m, c, s, a)
    M, g = get_mds_matrix(p, m)
    rc = get_round_constants(p, m, c, s, N)
    return dict(p=p, m=m, c=c, s=s, alpha=a, alphainv=ai, N=N, MDS=M, rc=rc, g=g)

def perm(P, st):
    p, m, N, M, rc = P["p"], P["m"], P["N"], P["MDS"], P["rc"]
    st = list(st)
    for i in range(N):
        st = [pow(x, P["alpha"], p) for x in st]
        st = [sum(M[r][k]*st[k] for k in range(m)) % p for r in range(m)]
        st = [(st[j] + rc[i*2*m+j]) % p for j in range(m)]
        st = [pow(x, P["alphainv"], p) for x in st]
        st = [sum(M[r][k]*st[k] for k in range(m)) % p for r in range(m)]
        st = [(st[j] + rc[i*2*m+m+j]) % p for j in range(m)]
    return st

def hash_(P, inp):
    rate = P["m"] - P["c"]; p = P["p"]
    x = list(inp) + [1]
    while len(x) % rate: x.append(0)
    st = [0]*P["m"]
    for b in range(0, len(x), rate):
        for i in range(rate): st[i] = (st[i] + x[b+i]) % p
        st = perm(P, st)
    return st[:rate]
