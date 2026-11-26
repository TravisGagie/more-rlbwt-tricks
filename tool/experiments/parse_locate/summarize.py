#!/usr/bin/env python3
# Summary of run.sh's per-MEM tables (results/chr19/parse_locate/*.tsv.gz; columns in src/surv_count.hpp).
import gzip, math, sys, os
R = sys.argv[1] if len(sys.argv) > 1 else 'results/chr19/parse_locate'
def load(name, N, L):
    rows = []
    with gzip.open(f'{R}/{name}.{N}.{L}.tsv.gz', 'rt') as f:
        for line in f:
            a, e, kind, occ, surv, brows, plen, left, why, lch, blocks = map(int, line.split('\t'))
            rows.append(dict(a=a, e=e, kind=kind, occ=occ, surv=surv, brows=brows, plen=plen, left=left, why=why, lch=lch))
    return rows
def q(v, p):                       # nearest rank
    v = sorted(v); return v[max(0, math.ceil(p * len(v)) - 1)]
out = []
P = out.append
P('| individuals, L | MEMs | no parse level | right end only | with parse steps | sum of occ | sum of survivors | survivors = occ | survivors / sum occ |')
P('|---|---|---|---|---|---|---|---|---|')
T = {}
for N, ind in ((33, 16), (65, 32), (129, 64)):
    for L in (1000, 100):
        r = load('blocks', N, L); T[N, L] = r
        k2 = [x for x in r if x['kind'] == 2]
        so, ss = sum(x['occ'] for x in k2), sum(x['surv'] for x in k2)
        P(f'| {ind}, {L} | {len(r):,} | {sum(x["kind"]==0 for x in r):,} | {sum(x["kind"]==1 for x in r):,} | {len(k2):,} | '
          f'{sum(x["occ"] for x in r):,} ({so:,}) | {ss:,} | {sum(x["surv"]==x["occ"] for x in k2):,} | {ss/so:.3f} |')
P('')
for title, f in (('occ', lambda x: x['occ']), ('survivors', lambda x: x['surv']), ('survivors / occ', lambda x: x['surv'] / x['occ']),
                 ('survivors - occ (the almost-MEMs)', lambda x: x['surv'] - x['occ'])):
    P(f'**{title}**, over the MEMs with parse steps:')
    P('')
    P('| individuals, L | mean | median | 90% | 99% | maximum |')
    P('|---|---|---|---|---|---|')
    for (N, L), r in T.items():
        v = [f(x) for x in r if x['kind'] == 2]
        fmt = (lambda y: f'{y:.3g}') if title.startswith('survivors /') else (lambda y: f'{y:,.0f}')
        P(f'| {N//2}, {L} | {sum(v)/len(v):.2f} | {fmt(q(v,.5))} | {fmt(q(v,.9))} | {fmt(q(v,.99))} | {fmt(max(v))} |')
    P('')
P('Block rows and the left piece (MEMs with parse steps):')
P('')
P('| individuals, L | sum of block rows among the survivors | survivors that are not block rows (MEMs, rows) | left piece: phrases of an occurrence, mean / max | stopped at pid = 0 | left piece in characters, mean / max | phrases crossed, mean / min |')
P('|---|---|---|---|---|---|---|')
for (N, L), r in T.items():
    k2 = [x for x in r if x['kind'] == 2]
    nb = [x for x in k2 if x['brows'] != x['surv']]
    P(f'| {N//2}, {L} | {sum(x["brows"] for x in k2):,} | {len(nb):,}, {sum(x["surv"]-x["brows"] for x in nb):,} | {sum(x["left"]+1 for x in k2)/len(k2):.3f} / {max(x["left"]+1 for x in k2)} | '
      f'{sum(x["why"] for x in k2):,} | {sum(x["lch"] for x in k2)/len(k2):.1f} / {max(x["lch"] for x in k2)} | {sum(x["plen"] for x in k2)/len(k2):.1f} / {min(x["plen"] for x in k2)} |')
P('')
P('▷ and heavy paths theta = 80% against two-level search with blocks, 16 individuals, MEM by MEM:')
P('')
for L in (1000, 100):
    b = T[33, L]
    for name in ('unique', 'heavy08'):
        o = load(name, 33, L)
        assert [(x['a'], x['e']) for x in o] == [(x['a'], x['e']) for x in b]
        d = sum(x['surv'] != y['surv'] or x['kind'] != y['kind'] or x['left'] != y['left'] for x, y in zip(o, b))
        P(f'- L = {L}, {name}: {len(o):,} MEMs, kind, survivors or left piece differ in {d}; sum of survivors {sum(x["surv"] for x in o if x["kind"]==2):,}')
# MEMs without parse steps: their lengths
P('')
for (N, L), r in T.items():
    k01 = [x for x in r if x['kind'] != 2]
    if k01: P(f'- {N//2}, {L}: MEMs without parse steps: {len(k01):,}, lengths {min(x["e"]-x["a"] for x in k01)}-{max(x["e"]-x["a"] for x in k01)}, occurrences {sum(x["occ"] for x in k01):,}; right end only: survivors {sum(x["surv"] for x in k01 if x["kind"]==1):,} for occ {sum(x["occ"] for x in k01 if x["kind"]==1):,}')
print('\n'.join(out))
