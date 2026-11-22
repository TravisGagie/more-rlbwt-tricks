#!/usr/bin/env python3
# Latency hiding on the move structure, part 2: the table of its timing session (experiments/latency/final.sh with
# ROWS "move move+chop move+co<K>... heavy08 heavy08+table"; logs q<N>.<row>.<alg>.<off|hp>.<round>.{log,hp}).
#   python3 p2_table.py LOGDIR
# Medians of 3 (†: of 5, when the 3 runs without huge pages differed by more than 3%), every run, the speedup of each
# K over the unchanged row and over chopping alone, the headline ratio (plain search over the best two-level row of
# the same session), and the shares of memory in huge pages.
import sys, re, glob, os, statistics, collections
L = sys.argv[1]; IND = {33: 16, 65: 32, 129: 64}; NS = [33, 65, 129]
runs = collections.defaultdict(list); share = collections.defaultdict(list)
for f in sorted(glob.glob(f"{L}/q*.log")):
    if not os.path.exists(f[:-4] + ".hp") or not os.path.getsize(f[:-4] + ".hp"): continue
    n, row, a, m, r = re.match(r"q(\d+)\.(.+)\.(bml|fb)\.(off|hp)\.(\d+)\.log$", os.path.basename(f)).groups()
    t = float(re.search(r"\(total ([\d.]+) s\)", open(f).read()).group(1))
    runs[(int(n), row, a, m)].append((int(r), t))
    share[(int(n), row, m)].append(float(re.search(r"huge pages ([\d.]+)%", open(f[:-4] + ".hp").read()).group(1)))
rows = []
for (n, row, a, m) in runs:
    if row not in rows: rows.append(row)
order = ["move", "move+chop"] + sorted([r for r in rows if r.startswith("move+co")], key=lambda r: int(r[7:])) + ["heavy08", "heavy08+table"]
rows = [r for r in order if r in rows]
NAME = {"move": "plain, move structure, unchanged", "move+chop": "chopped (64 pieces), one by one", "heavy08": "heavy paths θ = 80%",
        "heavy08+table": "heavy paths θ = 80%, with the first-run table"}
EXTRA = {4, 6}                       # beyond K = 8, 16, 32
def name(r):
    if r.startswith("move+co"):
        k = int(r[7:]); return f"chopped and interleaved, K = {k}" + (" (extra)" if k in EXTRA else "")
    return NAME[r]
def med(n, row, a, m):
    v = [t for _, t in sorted(runs[(n, row, a, m)])]
    return statistics.median(v), "†" if len(v) == 5 else ""
MODES = (("off", "without huge pages"), ("hp", "with huge pages"))
print("# Part 2: chopped and interleaved plain search on the move structure\n")
for m, mn in MODES:
    print(f"## Times, {mn} (s, BML / FB, median of 3; 16 / 32 / 64 individuals)\n")
    print("| row | 16 | 32 | 64 |\n|---|---:|---:|---:|")
    for r in rows:
        print(f"| {name(r)} | " + " | ".join("{:.2f}{} / {:.2f}{}".format(*med(n, r, 'bml', m), *med(n, r, 'fb', m)) for n in NS) + " |")
    print()
for m, mn in MODES:
    print(f"## Speedups, {mn}: unchanged / row, and chopped alone / row (BML / FB)\n")
    print("| row | 16 | 32 | 64 |\n|---|---|---|---|")
    for r in rows:
        if not r.startswith("move+"): continue
        c = []
        for n in NS:
            u = [med(n, "move", a, m)[0] / med(n, r, a, m)[0] for a in ("bml", "fb")]
            h = [med(n, "move+chop", a, m)[0] / med(n, r, a, m)[0] for a in ("bml", "fb")]
            c.append(f"{u[0]:.2f} / {u[1]:.2f} (over chopped {h[0]:.2f} / {h[1]:.2f})")
        print(f"| {name(r)} | " + " | ".join(c) + " |")
    print()
ks = [r for r in rows if r.startswith("move+co")]
req = [r for r in ks if int(r[7:]) not in EXTRA]
print("## Headline ratios: plain search over the best two-level row of this session (heavy paths θ = 80% with the table)\n")
print("| mode | algorithm | plain search | 16 | 32 | 64 |\n|---|---|---|---:|---:|---:|")
for m, mn in MODES:
    for a in ("bml", "fb"):
        for lab, cand in (("unchanged", ["move"]), ("best of K = 8, 16, 32", req), ("best K, with the extra K", ks)):
            c = []
            for n in NS:
                b = min(cand, key=lambda r: med(n, r, a, m)[0])
                c.append(f"{med(n, b, a, m)[0] / med(n, 'heavy08+table', a, m)[0]:.2f}" + ("" if lab == "unchanged" else f" (K = {b[7:]})"))
            print(f"| {mn} | {a.upper()} | {lab} | " + " | ".join(c) + " |")
print()
print("## Every run (s)\n")
print("| row | individuals | algorithm | mode | runs |\n|---|---:|---|---|---|")
for r in rows:
    for n in NS:
        for a in ("bml", "fb"):
            for m, mn in MODES:
                v = [t for _, t in sorted(runs[(n, r, a, m)])]
                sp = (max(v[:3]) - min(v[:3])) / min(v[:3])
                print(f"| {name(r)} | {IND[n]} | {a.upper()} | {mn} | " + ", ".join(f"{t:.2f}" for t in v) + f" (spread {100 * sp:.1f}%) |")
print()
print("## Shares of resident memory in huge pages (with huge pages; range over the runs)\n")
print("| row | 16 | 32 | 64 |\n|---|---:|---:|---:|")
for r in rows:
    print(f"| {name(r)} | " + " | ".join(f"{min(share[(n, r, 'hp')]):.0f}–{max(share[(n, r, 'hp')]):.0f}%" for n in NS) + " |")
