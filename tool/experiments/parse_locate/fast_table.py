#!/usr/bin/env python3
"""Tables of experiments/parse_locate/fast_time.sh: medians, spreads, added times, peaks.
   fast_table.py TIMES.txt > tables.md   (lines: N row config alg round seconds peakKB)"""
import sys, statistics as S
from collections import defaultdict
T = defaultdict(list); P = defaultdict(list)
for line in open(sys.argv[1]):
    f = line.split()
    if len(f) < 7: continue
    N, row, c, a, r, t, pk = f
    T[(N, row, c, a)].append(float(t)); P[(N, row, c, a)].append(int(pk))
NS = [n for n in ["33", "65", "129"] if any(k[0] == n for k in T)]
IND = {"33": 16, "65": 32, "129": 64}
ROWS = ["tl", "tl+table+la2", "tlur+table+la2", "tlh08+table"]
NAME = {"tl": "two-level", "tl+table+la2": "with the table and lookahead", "tlur+table+la2": "▷, jump, tables and lookahead",
        "tlh08+table": "heavy paths θ = 80%, with the table"}
PAPER = {  # tables.md of ce3117d (huge pages), BML / FB at 16 / 32 / 64
    "tl": [(5.00, 5.54), (5.49, 5.92), (6.20, 6.66)], "tl+table+la2": [(3.40, 3.71), (3.63, 3.95), (4.00, 4.33)],
    "tlur+table+la2": [(2.50, 2.67), (2.67, 2.86), (2.94, 3.15)], "tlh08+table": [(2.26, 2.37), (2.25, 2.38), (2.32, 2.47)]}
med = lambda k: S.median(T[k]) if T.get(k) else None
spr = lambda k: (max(T[k]) - min(T[k])) / min(T[k]) * 100 if T.get(k) else None
print("## Without locating against the paper's (huge pages), BML / FB, s\n")
print("| row | " + " | ".join(f"{IND[n]}: this session (spread) / paper" for n in NS) + " |")
print("|---|" + "---|" * len(NS))
for row in ROWS:
    cells = []
    for i, n in enumerate(NS):
        x = []
        for j, a in enumerate(["bml", "fb"]):
            k = (n, row, "off", a)
            if med(k) is None: x.append("–"); continue
            x.append(f"{med(k):.2f} ({spr(k):.1f}%) / {PAPER[row][[33,65,129].index(int(n))][j]:.2f}")
        cells.append("; ".join(x))
    print(f"| {NAME[row]} | " + " | ".join(cells) + " |")
print("\n## Added time, s = 64 (median without → with: added), s\n")
print("| row | alg | " + " | ".join(str(IND[n]) for n in NS) + " |")
print("|---|---|" + "---|" * len(NS))
for row in ROWS:
    for a in ["bml", "fb"]:
        cells = []
        for n in NS:
            o, w = med((n, row, "off", a)), med((n, row, "on", a))
            cells.append("–" if o is None or w is None else f"{o:.3f} → {w:.3f}: **{w - o:+.3f}**")
        print(f"| {NAME[row]} | {a.upper()} | " + " | ".join(cells) + " |")
print("\n## Configurations on the rows tl and tlh08+table: added time against off (median), s\n")
CF = [("old", "8912148 (bin/katka-ploc0): one row at a time, after each MEM"), ("lanes0", "one row at a time, per segment"),
      ("rows", "12 staged lanes, a row at a time"), ("on", "12 staged lanes, a run of rows at a time (final), s = 64"),
      ("s32", "final, s = 32"), ("s16", "final, s = 16"), ("nowalk", "rows collected, not walked")]
print("| row | configuration | alg | " + " | ".join(str(IND[n]) for n in NS) + " |")
print("|---|---|---|" + "---|" * len(NS))
for row in ["tl", "tlh08+table"]:
    for c, d in CF:
        for a in ["bml", "fb"]:
            cells = []
            for n in NS:
                o, w = med((n, row, "off", a)), med((n, row, c, a))
                cells.append("–" if o is None or w is None else f"{w - o:+.3f} (spread {spr((n, row, c, a)):.1f}%)")
            if all(x == "–" for x in cells): continue
            print(f"| {NAME[row]} | {d} | {a.upper()} | " + " | ".join(cells) + " |")
print("\n## Peak resident size (MB, largest of the rounds)\n")
print("| row | configuration | " + " | ".join(str(IND[n]) for n in NS) + " |")
print("|---|---|" + "---|" * len(NS))
for row in ROWS:
    for c in ["off", "on", "s32", "s16"]:
        cells = [f"{max(P[(n,row,c,'bml')] + P[(n,row,c,'fb')]) / 1024:.0f}" if P.get((n, row, c, "bml")) else "–" for n in NS]
        if all(x == "–" for x in cells): continue
        print(f"| {NAME[row]} | {c} | " + " | ".join(cells) + " |")
print("\nLargest spread of any configuration: %.1f%%" % max(spr(k) for k in T))
