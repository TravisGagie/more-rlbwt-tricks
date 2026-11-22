#!/usr/bin/env python3
# Tables of experiments/latency/final.sh: python3 final_table.py LOGDIR > tables.md; also writes LOGDIR/../runs.tsv.
# Each of v3's Tables 1-9 twice (without and with huge pages), the per-run times and their spread, the shares of
# memory in huge pages, the sizes, and the headline ratios.  Medians of 3 runs; "†" marks a median of 5 (the 3 runs
# without huge pages differed by more than 3%, so final.sh ran two more).  Sizes in MB (10^6 bytes) of the files each
# search loads, as experiments/twolevel_ac_table.py (plus the 8-mer table built with --move); the first-run table is
# built at load and is given separately.
import sys, os, re, glob, statistics, collections
L = sys.argv[1]; D = os.environ.get("CHR19", os.path.expanduser("~/research/rlbwt-tricks/chr19"))
T = os.environ.get("T", os.path.join(os.path.dirname(L.rstrip("/")), "tools"))
NS = [33, 65, 129]; IND = {33: 16, 65: 32, 129: 64}
NAME = {"rlcsa": "RLCSA", "move": "our move structure", "ropebwt3": "ropebwt3", "movi2": "Movi 2",
        "nojumps.csa": "two-level, RLCSA", "nojumps.move": "two-level, move structure", "stpdindex": "STPD-index",
        "stpd": "STPD of the parse", "tsc25t0": "almost-funnels, windows of 25", "tsc100t0": "almost-funnels, windows of 100",
        "tsc25t6": "almost-funnels, t ≤ 6", "ref4": "almost-funnels, g = 4", "ref16": "almost-funnels, g = 16",
        "ref64": "almost-funnels, g = 64", "heavy08": "θ = 80%", "heavy09": "θ = 90%", "heavy095": "θ = 95%",
        "unique": "with encoding (▷)"}
VAR = {"table": "with the first-run table", "table+la2": "with the table and lookahead (d = 2)", "la4": "with lookahead alone (d = 4)"}
def name(r):
    b, _, v = r.partition("+")
    return NAME[b] + (", " + VAR[v] + " (new)" if v else "")
TABLES = [
    ("Table 1", ["rlcsa", "move", "ropebwt3", "movi2"]),
    ("Table 2", ["nojumps.csa", "nojumps.move", "nojumps.move+table", "nojumps.move+table+la2", "nojumps.move+la4"]),
    ("Table 3", ["stpdindex"]),
    ("Table 4", ["stpd"]),
    ("Table 5", ["move", "tsc25t0", "tsc100t0"]),
    ("Table 6", ["move", "tsc25t6", "nojumps.move", "ref4", "ref16", "ref64"]),
    ("Table 7", ["nojumps.move", "heavy08", "heavy08+table", "heavy09", "heavy09+table", "heavy095", "heavy095+table"]),
    ("Table 8", ["nojumps.move", "unique", "unique+table", "unique+table+la2"]),
]
SUMMARY = ["ropebwt3", "movi2", "stpdindex", "rlcsa", "move", "tsc25t0", "tsc100t0", "tsc25t6", "nojumps.move",
           "nojumps.move+table", "nojumps.move+table+la2", "nojumps.move+la4", "stpd", "ref64", "ref16", "ref4",
           "heavy095", "heavy095+table", "heavy09", "heavy09+table", "heavy08", "heavy08+table", "unique", "unique+table",
           "unique+table+la2"]
TWOLEVEL = [r for r in SUMMARY if r.split("+")[0] in ("nojumps.move", "ref4", "ref16", "ref64", "heavy08", "heavy09", "heavy095", "unique")]

def tval(row, s):
    if row == "ropebwt3":
        l = float(re.search(r"::([\d.]+)\*[^\n]*loaded the BWT", s).group(1)); p = float(re.search(r"::([\d.]+)\*[^\n]*processed", s).group(1))
        return p - l
    if row == "movi2": return float(re.search(r"processing the reads: ([\d.]+)", re.sub(r"\x1b\[[0-9;]*m", "", s)).group(1))
    if row == "stpdindex": return float(re.search(r"finding them ([\d.]+) s", s).group(1))
    return float(re.search(r"\(total ([\d.]+) s\)", s).group(1))

runs = collections.defaultdict(list); share = collections.defaultdict(list); first = {}
out = open(os.path.join(os.path.dirname(L.rstrip("/")), "runs.tsv"), "w")
out.write("individuals\trow\talgorithm\tmode\tround\tseconds\thuge_page_share_pct\tresident_MB\tpeak_KB\n")
for f in sorted(glob.glob(f"{L}/q*.log")):
    n, row, alg, mode, r = re.match(r"q(\d+)\.(.+)\.(bml|fb|all)\.(off|hp)\.(\d+)\.log$", os.path.basename(f)).groups()
    if not os.path.exists(f[:-4] + ".hp") or not os.path.getsize(f[:-4] + ".hp"): continue   # still running
    n, r = int(n), int(r); s = open(f).read(); t = tval(row, s)
    h = open(f[:-4] + ".hp").read(); m = re.search(r"huge pages ([\d.]+)% of (\d+) MB", h)
    pk = re.search(r"peak (\d+) KB", open(f[:-4] + ".time").read()).group(1)
    runs[(n, row, alg, mode)].append((r, t)); share[(n, row, mode)].append(float(m.group(1)))
    out.write(f"{IND[n]}\t{row}\t{alg}\t{mode}\t{r}\t{t:.3f}\t{m.group(1)}\t{m.group(2)}\t{pk}\n")
    fm = re.search(r"first-run tables: parse ([\d.]+) MB, blocks ([\d.]+) MB", s)
    if fm: first[(n, row)] = float(fm.group(1)) + float(fm.group(2))
out.close()

def med(n, row, alg, mode):
    v = [t for _, t in sorted(runs.get((n, row, alg, mode), []))]
    if not v: return None, ""
    return statistics.median(v), ("†" if len(v) == 5 else "" if len(v) == 3 else f" ({len(v)} runs)")
def cell(n, row, mode, algs=("bml", "fb")):
    if (n, row, algs[0], mode) not in runs: return "—"
    a = algs[1] if row != "movi2" else "all"
    (b, mb), (f, mf) = med(n, row, algs[0], mode), med(n, row, a, mode)
    if b is None or f is None: return "—"
    return f"{b:.2f}{mb} / {f:.2f}{mf}" + ("∗" if row == "movi2" else "")

def sz(p): return os.stat(p).st_size
TABLE8 = 65536 * 32
def mb(n, row):
    b = row.split("+")[0]
    if b == "ropebwt3": return sz(f"{T}/n{IND[n]}/rb3/index.fmd") / 1e6
    if b == "movi2": return sum(sz(f"{T}/n{IND[n]}/movi/{e}") for e in ("index.movi", "ftab.10.bin")) / 1e6
    if b == "stpdindex":
        if n == 129: return None
        x = open(os.path.expanduser("~/research/mem-finding-latency/tool/results/chr19/stpd_index_rerun/summary.txt")).read()
        return float(re.search(rf"c{n} \(\d+ individuals\) RLZ-r-index: .*?MEM finding loads the first two, ([\d.]+) MB", x).group(1))
    base, t = (b.split(".") + ["csa"])[:2]
    if base == "rlcsa": return sz(f"{D}/c{n}.csa") / 1e6
    if base == "move": return (sz(f"{D}/c{n}.mv") + TABLE8) / 1e6
    if base.startswith("tsc"): return (sz(f"{D}/c{n}.mv") + TABLE8 + sz(f"{D}/tsc/c{n}.ell{base[3:]}.tsc")) / 1e6
    if base == "stpd": return sum(sz(f"{D}/c{n}.{e}") for e in ("csa", "lvl", "pstpd", "ptxt", "rlbwt", "stpd")) / 1e6
    t = "move" if base in ("ref4", "ref16", "ref64", "heavy08", "heavy09", "heavy095", "unique") else t
    x = sz(f"{D}/c{n}.rlbwt") + (sz(f"{D}/c{n}.csa") if t == "csa" else sz(f"{D}/c{n}.mv") + TABLE8)
    x += sz(f"{D}/unique/c{n}u.lvl") if base == "unique" else sz(f"{D}/c{n}.lvl")
    if base.startswith("heavy") or base.startswith("ref"): x += sz(f"{D}/psc/c{n}.{base}.psc")
    return x / 1e6
def mbs(row): return " / ".join("—" if mb(n, row) is None else f"{mb(n, row):,.0f}" for n in NS)

MODES = (("off", "without huge pages"), ("hp", "with huge pages (GLIBC_TUNABLES=glibc.malloc.hugetlb=1)"))
print("# Final timing session: every timed table of v3, without and with huge pages\n")
print("Seconds, BML / forward–backward, median of 3 (†: median of 5, see the per-run times); MB of the files each search "
      "loads (10^6 bytes; the paper's MB column), with 16 / 32 / 64 individuals. ∗ Movi 2's own query for all MEMs. "
      "New rows are marked (new); their files are their base row's, and the first-run table they build at load is in "
      "\"Sizes\" below.\n")
for t, rows in TABLES:
    for mode, mname in MODES:
        print(f"## {t}, {mname}\n")
        print("| | MB | 16 | 32 | 64 |\n|---|---|---:|---:|---:|")
        for row in rows: print(f"| {name(row)} | {mbs(row)} | " + " | ".join(cell(n, row, mode) for n in NS) + " |")
        print()
for mode, mname in MODES:
    print(f"## Table 9 (summary, BML), {mname}\n")
    print("| index, method | 16 | 32 | 64 | MB |\n|---|---:|---:|---:|---|")
    for row in SUMMARY:
        print(f"| {name(row)} | " + " | ".join("—" if med(n, row, "bml", mode)[0] is None else f"{med(n, row, 'bml', mode)[0]:.2f}{med(n, row, 'bml', mode)[1]}" for n in NS) + f" | {mbs(row)} |")
    print()

print("## Headline ratios: plain move structure over the best two-level row\n")
print("| mode | algorithm | rows | 16 | 32 | 64 |\n|---|---|---|---:|---:|---:|")
for mode, mname in MODES:
    for alg in ("bml", "fb"):
        for lab, cand in (("the paper's rows", [r for r in TWOLEVEL if "+" not in r]), ("with the new rows", TWOLEVEL)):
            c = []
            for n in NS:
                cand_n = [r for r in cand if med(n, r, alg, mode)[0] is not None]
                if not cand_n or med(n, "move", alg, mode)[0] is None: c.append("—"); continue
                best = min(cand_n, key=lambda r: med(n, r, alg, mode)[0]); c.append(f"{med(n, 'move', alg, mode)[0] / med(n, best, alg, mode)[0]:.2f} ({name(best)})")
            print(f"| {mname.split(' (')[0]} | {alg.upper()} | {lab} | " + " | ".join(c) + " |")
print()

print("## Stability\n")
print("Spread = (max − min) / min of the first 3 runs of each row, size and algorithm.\n")
print("| mode | cells | over 3% | over 1% | median spread | largest spread |\n|---|---:|---:|---:|---:|---|")
for mode, mname in MODES:
    sp = []
    for (n, row, alg, m), v in runs.items():
        if m != mode: continue
        x = [t for r, t in v if r <= 3]; sp.append(((max(x) - min(x)) / min(x), f"{name(row)}, {alg.upper()}, {IND[n]}"))
    sp.sort()
    if not sp: continue
    print(f"| {mname.split(' (')[0]} | {len(sp)} | {sum(s > 0.03 for s, _ in sp)} | {sum(s > 0.01 for s, _ in sp)} | "
          f"{100 * statistics.median(s for s, _ in sp):.2f}% | {100 * sp[-1][0]:.1f}% ({sp[-1][1]}) |")
print()
print("Cells over 3% without huge pages (all runs, s):\n")
for (n, row, alg, m), v in sorted(runs.items(), key=lambda kv: (kv[0][0], SUMMARY.index(kv[0][1]) if kv[0][1] in SUMMARY else 99)):
    if m != "off": continue
    x = [t for r, t in sorted(v) if r <= 3]
    if (max(x) - min(x)) / min(x) > 0.03:
        print(f"- {IND[n]}, {name(row)}, {alg.upper()}: " + ", ".join(f"{t:.2f}" for _, t in sorted(v)) +
              f"; with huge pages " + ", ".join(f"{t:.2f}" for _, t in sorted(runs[(n, row, alg, 'hp')])))
print()

print("## Shares of resident memory in huge pages (with huge pages; range over all runs of the row)\n")
print("| row | 16 | 32 | 64 |\n|---|---:|---:|---:|")
for row in SUMMARY:
    c = []
    for n in NS:
        v = share.get((n, row, "hp"))
        c.append("—" if not v else f"{min(v):.0f}–{max(v):.0f}%" if max(v) - min(v) >= 0.5 else f"{min(v):.0f}%")
    print(f"| {name(row)} | " + " | ".join(c) + " |")
print()
print("## Sizes of the new rows\n")
print("| row | MB of files loaded (16 / 32 / 64) | first-run table built at load, MB (16 / 32 / 64) |\n|---|---|---|")
for row in SUMMARY:
    if "+" not in row: continue
    print(f"| {name(row)} | {mbs(row)} | " + " / ".join(f"{first[(n, row)]:.1f}" if (n, row) in first else "—" for n in NS) + " |")
