#!/usr/bin/env python3
# A.3 of the latency study: longest MEMs (L = 1000) in the paper's BML output blocks/q<N>s.blocks.tsv (columns pattern,
# start, end exclusive, sp, ep), and the sum over MEMs of M^2 / (2B) for piece lengths B = total / 64, 128, 256.
#   python3 a3_mems.py CHR19
import sys
D = sys.argv[1]
tot = 0
for l in open(f"{D}/blocks/queries.fa"):
    if l[0] != ">": tot += len(l.strip())
print(f"pattern length, all four patterns: {tot}")
for n, sz in ((33, 16), (65, 32), (129, 64)):
    L = sorted((int(f[2]) - int(f[1]) for f in (l.split("\t") for l in open(f"{D}/blocks/q{n}s.blocks.tsv"))), reverse=True)
    sq = sum(x * x for x in L)
    print(f"{sz} individuals: {len(L)} MEMs, max {L[0]}, ten longest {L[:10]}")
    for p in (64, 128, 256):
        B = tot / p
        print(f"   {p} pieces, B = {B:,.0f}: sum M^2/(2B) = {sq / (2 * B):,.0f} characters, {100 * sq / (2 * B) / tot:.4f}% of the pattern length")
