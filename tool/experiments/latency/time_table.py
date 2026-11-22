#!/usr/bin/env python3
# Medians of experiments/latency/time.sh's runs: python3 time_table.py DIR [DIR ...] (wall times of find, load excluded)
import sys, glob, re, statistics, collections
t = collections.defaultdict(list); mb = {}
for d in sys.argv[1:]:
    for f in glob.glob(f"{d}/*.log"):
        q, row, lab, alg, r = f.split("/")[-1][:-4].split(".")
        s = open(f).read(); t[(row, lab, q, alg)].append(float(re.search(r"wall ([0-9.]+) s", s).group(1)))
        m = re.search(r"first-run tables: parse ([0-9.]+) MB, blocks ([0-9.]+) MB", s)
        if m: mb[(row, lab, q)] = (float(m.group(1)), float(m.group(2)))
rows = sorted({(k[0], k[1]) for k in t})
print("| row | variant | 16: BML / FB (s) | 32: BML / FB (s) | 64: BML / FB (s) | first-run tables, MB at 16 / 32 / 64 |")
print("|---|---|---:|---:|---:|---|")
for row, lab in rows:
    cells = []
    for q in ("q33", "q65", "q129"):
        b, f = t.get((row, lab, q, "bml")), t.get((row, lab, q, "fb"))
        cells.append(f"{statistics.median(b):.2f} / {statistics.median(f):.2f}" if b and f else "")
    mbs = " / ".join(f"{mb[(row, lab, q)][0] + (mb[(row, lab, q)][1] if row == 'unique' else 0):.1f}" for q in ("q33", "q65", "q129") if (row, lab, q) in mb)
    print(f"| {row} | {lab} | " + " | ".join(cells) + f" | {mbs} |")
print()
for k in sorted(t): print(" ".join(k), " ".join(f"{x:.2f}" for x in t[k]))
