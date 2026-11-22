#!/usr/bin/env python3
# A.1 / A.2 histograms of the latency study from a_counts.sh's output:
#   python3 a_hist.py W
# For each index (pbwt of c<N>, pbwt and bbwt of c<N>u) and each size: runs per symbol and bytes per symbol (KS low
# bits + FP low bits of its runs), over symbols and weighted by the steps of the search (BML, L = 1000), and the
# fraction of steps whose symbol's range fits in one or two 64-byte lines of KS's low bits, and in one line of each of
# the four arrays (KS low, KS high, FP low, FP high).
import sys, numpy as np
W = sys.argv[1]
RB = [(1, 1), (2, 2), (3, 4), (5, 8), (9, 16), (17, 64), (65, 256), (257, 1 << 62)]
BB = [(0, 8), (9, 16), (17, 32), (33, 64), (65, 128), (129, 256), (257, 1024), (1025, 1 << 62)]
def lab(a, b): return f"{a}" if a == b else (f">{a - 1}" if b >= 1 << 62 else f"{a}-{b}")
def load(tsv):
    h = open(tsv).readline().split()
    kl, fl = int(h[h.index("KS.l") + 1]), int(h[h.index("FP.l") + 1])
    a = np.loadtxt(tsv, dtype=np.int64, skiprows=2)
    return a, kl, fl
def hist(vals, w, bins):
    t = w.sum(); return [100.0 * w[(vals >= a) & (vals <= b)].sum() / t for a, b in bins]
for n, sz in ((33, 16), (65, 32), (129, 64)):
    for tag, tsv, steps in ((f"c{n} pbwt (plain)", f"c{n}.pbwt.tsv", f"c{n}.nojumps.pbwt.steps"),
                            (f"c{n}u pbwt (unique)", f"c{n}u.pbwt.tsv", f"c{n}u.unique.pbwt.steps"),
                            (f"c{n}u bbwt (unique)", f"c{n}u.bbwt.tsv", f"c{n}u.unique.bbwt.steps")):
        a, kl, fl = load(f"{W}/{tsv}")
        c, r = a[:, 0], a[:, 1]
        st = np.fromfile(f"{W}/{steps}", dtype=np.uint64).astype(np.int64)
        w = np.zeros(len(c), dtype=np.int64); ok = c < len(st); w[ok] = st[c[ok]]
        byt = (r * (kl + fl) + 7) // 8
        one = (a[:, 2] <= 1); two = (a[:, 2] <= 2); all1 = (a[:, 2] <= 1) & (a[:, 3] <= 1) & (a[:, 4] <= 1) & (a[:, 5] <= 1)
        T = w.sum()
        print(f"== {sz} individuals, {tag}: {len(c)} symbols, {r.sum()} runs (KS.l {kl}, FP.l {fl}), {T} steps "
              f"({100.0 * w[w > 0].size / len(c):.1f}% of symbols stepped on)")
        print("  runs:  " + "  ".join(f"{lab(*b)}" for b in RB))
        print("   symbols %: " + "  ".join(f"{x:.2f}" for x in hist(r, np.ones_like(r), RB)))
        print("   steps %:   " + "  ".join(f"{x:.2f}" for x in hist(r, w, RB)))
        print("  bytes: " + "  ".join(f"{lab(*b)}" for b in BB))
        print("   symbols %: " + "  ".join(f"{x:.2f}" for x in hist(byt, np.ones_like(byt), BB)))
        print("   steps %:   " + "  ".join(f"{x:.2f}" for x in hist(byt, w, BB)))
        print(f"  steps whose range of KS low bits fits in 1 line: {100.0 * w[one].sum() / T:.2f}%, in 2: {100.0 * w[two].sum() / T:.2f}%;"
              f" one line in each of KS low, KS high, FP low, FP high: {100.0 * w[all1].sum() / T:.2f}%")
        print(f"  mean runs per step: {(w * r).sum() / T:.2f}; max runs: {r.max()}")
