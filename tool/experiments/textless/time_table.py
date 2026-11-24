#!/usr/bin/env python3
# Stage 2, part B5: the timing session of experiments/latency/final.sh with ROWS="nojumps.move unique textless
# nojumps.move+table+la2 unique+table+la2 textless+table+la2"; medians, the committed medians of the final timing session
# (results/chr19/latency/final/runs.tsv, cf143c3) next to them, and the largest resident size the probe saw.
#   python3 time_table.py LOGDIR COMMITTED_RUNS_TSV > times.md   (also writes LOGDIR/../runs.tsv)
import sys, os, re, glob, statistics, collections
L, C = sys.argv[1], sys.argv[2]; NS = [(33, 16), (65, 32), (129, 64)]
ROWS = [("nojumps.move", "two-level, move structure (the paper's)"), ("textless", "two-level, **without the index of the text**"),
        ("unique", "▷, move structure (the paper's)"), ("nojumps.move+table+la2", "two-level, table + lookahead d = 2"),
        ("textless+table+la2", "two-level without the index of the text, table + lookahead d = 2"), ("unique+table+la2", "▷, table + lookahead d = 2")]
t = collections.defaultdict(list); rss = collections.defaultdict(list); out = []
for f in sorted(glob.glob(f"{L}/q*.log")):
    m = re.match(r"q(\d+)\.(.+)\.(bml|fb)\.(off|hp)\.(\d+)\.log$", os.path.basename(f))
    if not m: continue
    n, row, a, mode, r = int(m[1]), m[2], m[3], m[4], int(m[5])
    s = float(re.search(r"\(total ([\d.]+) s\)", open(f).read())[1])
    h = re.search(r"huge pages ([\d.]+)% of (\d+) MB", open(f[:-4] + ".hp").read())
    t[(n, row, a, mode)].append(s); rss[(n, row, a, mode)].append(int(h[2])); out.append((dict(NS)[n], row, a, mode, r, s, h[1], h[2]))
with open(os.path.join(os.path.dirname(L.rstrip("/")), "runs.tsv"), "w") as o:
    o.write("individuals\trow\talgorithm\tmode\tround\tseconds\thuge_page_share_pct\tresident_MB\n")
    for x in out: o.write("\t".join(map(str, x)) + "\n")
c = collections.defaultdict(list)
for l in open(C).read().splitlines()[1:]:
    i, row, a, mode, r, s = l.split("\t")[:6]; c[(int(i), row, a, mode)].append(float(s))
med = lambda v: statistics.median(v) if v else None
for mode, title in (("off", "without huge pages"), ("hp", "with huge pages (GLIBC_TUNABLES=glibc.malloc.hugetlb=1)")):
    print(f"## Seconds, BML, {title}\n\nMedian of 3 (5 where final.sh ran two more); in parentheses the committed median; then the largest "
          "resident size (MB) during the run.\n\n| | 16 | 32 | 64 |\n|---|---:|---:|---:|")
    for row, nm in ROWS:
        cells = []
        for n, i in NS:
            v, k, r = med(t[(n, row, "bml", mode)]), med(c[(i, row, "bml", mode)]), max(rss[(n, row, "bml", mode)])
            cells.append(f"{v:.2f}" + (f" ({k:.2f})" if k else "") + f", {r:,} MB")
        print(f"| {nm} | " + " | ".join(cells) + " |")
    print("\n**Without the index of the text / with it** (this session)\n\n| | 16 | 32 | 64 |\n|---|---:|---:|---:|")
    for a, b, nm in (("textless", "nojumps.move", "plain"), ("textless+table+la2", "nojumps.move+table+la2", "table + lookahead")):
        print(f"| {nm} | " + " | ".join(f"{med(t[(n, a, 'bml', mode)]) / med(t[(n, b, 'bml', mode)]):.3f}" for n, _ in NS) + " |")
    print()
sp = [(max(v) - min(v)) / min(v) for v in t.values()]
print(f"Largest spread between the runs of one row, size and mode: {100 * max(sp):.1f}%. "
      f"Re-timed rows against the committed medians: " + ", ".join(f"{med(t[(n, row, a, m)]) / med(c[(i, row, a, m)]):.3f}"
      for row in ("nojumps.move", "unique", "nojumps.move+table+la2", "unique+table+la2") for n, i in NS for a in ("bml",) for m in ("off", "hp") if c[(i, row, a, m)]) + ".")
