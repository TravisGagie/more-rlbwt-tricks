#!/usr/bin/env python3
# Latency hiding on the move structure, part 2, checks: every MEM of length >= L of each pattern against the text S
# (both strands, as katka prep writes it), by brute force, compared with a katka find --format mems output (columns
# name, start, end).  Matching statistics by substring search: ms[a] = the length of the longest prefix of P[a..)
# occurring in S, with ms[a] >= ms[a-1] - 1; R[a..a+ms[a]) is a MEM if a = 0 or ms[a-1] < ms[a] + 1.
#   python3 -I p2_brute.py PREFIX.S QUERIES.fa L KATKA.tsv
import sys
S = open(sys.argv[1]).read(); L = int(sys.argv[3])
pats, name = {}, None
for l in open(sys.argv[2]):
    l = l.strip()
    if l.startswith(">"): name = l[1:].split()[0]; pats[name] = []
    elif name: pats[name].append(l)
want = set()
for name, parts in pats.items():
    P = "".join(parts); m = len(P); ms = [0] * m; l = 0
    for a in range(m):
        l = max(l - 1, 0)
        while a + l < m and P[a:a + l + 1] in S: l += 1
        ms[a] = l
    for a in range(m):
        if ms[a] >= L and (a == 0 or ms[a - 1] < ms[a] + 1): want.add((name, a, a + ms[a]))
got = set()
for l in open(sys.argv[4]):
    f = l.rstrip("\n").split("\t"); got.add((f[0], int(f[1]), int(f[2])))
print(f"brute force: {len(want)} MEMs of length >= {L}; katka: {len(got)}; only brute force {len(want - got)}, only katka {len(got - want)}")
sys.exit(0 if want == got else 1)
