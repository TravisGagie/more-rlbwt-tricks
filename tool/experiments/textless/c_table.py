#!/usr/bin/env python3
# Stage 2, part C's end table: the final session (experiments/latency/final.sh, logs in LOGDIR), in the paper's layout:
# MB as built (files loaded, plus the first-run table built at load for the table rows, as the paper counts), then
# seconds BML / FB, median of 3 (5 where final.sh ran two more), 16 / 32 / 64; the largest resident size per row; the
# re-timed rows of the paper against the committed medians (results/chr19/latency/final/runs.tsv, cf143c3).
#   python3 c_table.py LOGDIR COMMITTED_RUNS_TSV > tables.md   (also writes LOGDIR/../runs.tsv)
import sys, os, re, glob, statistics, collections
L, C = sys.argv[1], sys.argv[2]; NS = [(33, 16), (65, 32), (129, 64)]
D = os.path.expanduser("~/research/rlbwt-tricks/chr19"); DW = os.path.expanduser("~/diamond")
sz = lambda p: os.stat(p).st_size
TABLE8 = 65536 * 32
t = collections.defaultdict(list); rss = collections.defaultdict(list); ftab = {}; out = []
for f in sorted(glob.glob(f"{L}/q*.log")):
    m = re.match(r"q(\d+)\.(.+)\.(bml|fb)\.(off|hp)\.(\d+)\.log$", os.path.basename(f))
    if not m: continue
    n, row, a, mode, r = int(m[1]), m[2], m[3], m[4], int(m[5])
    s = open(f).read(); sec = float(re.search(r"\(total ([\d.]+) s\)", s)[1])
    fm = re.search(r"first-run tables: parse ([\d.]+) MB, blocks ([\d.]+) MB", s)
    if fm: ftab[(n, row)] = (float(fm[1]) + float(fm[2])) * 1e6
    h = re.search(r"huge pages ([\d.]+)% of (\d+) MB", open(f[:-4] + ".hp").read())
    t[(n, row, a, mode)].append(sec); rss[(n, row, mode)].append(int(h[2])); out.append((dict(NS)[n], row, a, mode, r, sec, h[1], h[2]))
with open(os.path.join(os.path.dirname(L.rstrip("/")), "runs.tsv"), "w") as o:
    o.write("individuals\trow\talgorithm\tmode\tround\tseconds\thuge_page_share_pct\tresident_MB\n")
    for x in out: o.write("\t".join(map(str, x)) + "\n")
c = collections.defaultdict(list)
for l in open(C).read().splitlines()[1:]:
    i, row, a, mode, r, s = l.split("\t")[:6]; c[(int(i), row, a, mode)].append(float(s))
med = lambda v: statistics.median(v) if v else None
def mb(n, row):
    b = row.split("+")[0]
    if b == "move": x = sz(f"{D}/c{n}.mv") + TABLE8
    elif b in ("nojumps.move", "unique", "heavy08"):
        x = sz(f"{D}/c{n}.rlbwt") + sz(f"{D}/c{n}.mv") + TABLE8 + sz(f"{D}/unique/c{n}u.lvl" if b == "unique" else f"{D}/c{n}.lvl")
        if b == "heavy08": x += sz(f"{D}/psc/c{n}.heavy08.psc")
    else:
        lvl = {"tlu": "utl", "tlur": "utl", "tlw": "wtl"}.get(b, "tl")
        x = sz(f"{DW}/c{n}{lvl}.lvl") + sz(f"{DW}/c{n}tl.tl2")
        if b.startswith("tlh") or b.startswith("tlr") and b != "tlur":
            p = b[2:].replace("h", "heavy").replace("r", "ref"); x += sz(f"{D}/psc/c{n}.{p}.psc")
    return (x + ftab.get((n, row), 0)) / 1e6
ROWS = [("move", "our move structure (paper's row, re-timed)"), ("nojumps.move", "two-level (paper's row, re-timed)"),
        ("unique", "▷ (paper's row, re-timed)"), ("heavy08", "heavy paths θ = 80% (paper's row, re-timed)"),
        ("heavy08+table", "heavy paths θ = 80%, with the table (paper's row, re-timed)"),
        ("tl", "**without the index of the text:** two-level"), ("tl+table+la2", "\\quad with the table and lookahead"),
        ("tlu", "\\quad ▷"), ("tlu+table+la2", "\\quad ▷, with the tables and lookahead"), ("tlur", "\\quad ▷, jump at the right end"),
        ("tlur+table+la2", "\\quad ▷, jump, tables and lookahead"), ("tlw", "\\quad ◇"),
        ("tlh08", "\\quad heavy paths θ = 80%"), ("tlh08+table", "\\quad heavy paths θ = 80%, with the table"),
        ("tlh09", "\\quad heavy paths θ = 90%"), ("tlh09+table", "\\quad heavy paths θ = 90%, with the table"),
        ("tlh095", "\\quad heavy paths θ = 95%"), ("tlh095+table", "\\quad heavy paths θ = 95%, with the table"),
        ("tlr4", "\\quad almost-funnels, g = 4"), ("tlr16", "\\quad almost-funnels, g = 16"), ("tlr64", "\\quad almost-funnels, g = 64")]
COMMITTED = {"move": "move", "nojumps.move": "nojumps.move", "unique": "unique", "heavy08": "heavy08", "heavy08+table": "heavy08+table"}
for mode, title in (("hp", "with huge pages (GLIBC_TUNABLES=glibc.malloc.hugetlb=1)"), ("off", "without huge pages")):
    print(f"## Seconds, BML / forward–backward, {title}\n\nMB as built (16 / 32 / 64; files loaded, plus the first-run table built at load); "
          "median of 3 (5 where the driver ran two more).\n\n| | MB | 16 | 32 | 64 |\n|---|---|---:|---:|---:|")
    for row, nm in ROWS:
        cells = [f"{med(t[(n, row, 'bml', mode)]):.2f} / {med(t[(n, row, 'fb', mode)]):.2f}" for n, _ in NS]
        print(f"| {nm} | " + " / ".join(f"{mb(n, row):,.0f}" for n, _ in NS) + " | " + " | ".join(cells) + " |")
    print(f"\n**The re-timed rows against the committed medians** (this session / cf143c3, BML / FB):\n\n| | 16 | 32 | 64 |\n|---|---:|---:|---:|")
    for row, cr in COMMITTED.items():
        print(f"| {row} | " + " | ".join(" / ".join(f"{med(t[(n, row, a, mode)]) / med(c[(i, cr, a, mode)]):.3f}" for a in ("bml", "fb")) for n, i in NS) + " |")
    print()
print("## The largest resident size (MB, with huge pages / without)\n\n| | 16 | 32 | 64 |\n|---|---:|---:|---:|")
for row, nm in ROWS: print(f"| {nm} | " + " | ".join(f"{max(rss[(n, row, 'hp')]):,} / {max(rss[(n, row, 'off')]):,}" for n, _ in NS) + " |")
sp = max((max(v) - min(v)) / min(v) for v in t.values())
print(f"\nLargest spread between the runs of one row, size, algorithm and mode: {100 * sp:.1f}%.")
