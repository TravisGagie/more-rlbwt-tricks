#!/usr/bin/env python3
# Stage 2, part C0: the MB column of every row of the posted paper that is built on the plain c<N>.lvl, as published
# (experiments/latency/final_table.py's rule) and corrected: the same files with c<N>.lvl replaced by a file without its
# pairs (bin/strip-pairs; DW/c<N>np.lvl), which the rows never read, and, for the rows with the first-run table, without
# the pairs' first-run table (the logs' "first-run tables: ... blocks X MB").  Every corrected size is a sum of file
# sizes (plus the table built at load, as in the paper), not a subtraction.
#   python3 c0_table.py > table.md
import os
D = os.path.expanduser("~/research/rlbwt-tricks/chr19"); DW = os.path.expanduser("~/diamond"); NS = [33, 65, 129]
LOGS = os.path.expanduser("~/data/latency/final/logs")
sz = lambda p: os.stat(p).st_size
TABLE8 = 65536 * 32
import re
def ft(n, row):        # the first-run tables (parse, blocks) of a table row, from the final timing session's log
    s = open(f"{LOGS}/q{n}.{row}.bml.off.1.log").read()
    m = re.search(r"first-run tables: parse ([\d.]+) MB, blocks ([\d.]+) MB", s); return float(m[1]) * 1e6, float(m[2]) * 1e6
def mb(n, row, lvl):
    base = row.split("+")[0]
    if base == "stpd": return sum(sz(f"{D}/c{n}.{e}") for e in ("csa", "pstpd", "ptxt", "rlbwt", "stpd")) + sz(lvl)
    x = sz(f"{D}/c{n}.rlbwt") + (sz(f"{D}/c{n}.csa") if base == "nojumps.csa" else sz(f"{D}/c{n}.mv") + TABLE8) + sz(lvl)
    if base.startswith("heavy") or base.startswith("ref"): x += sz(f"{D}/psc/c{n}.{base}.psc")
    return x
ROWS = [  # (paper's table, the paper's row label, final.sh row)
    ("Table 2 (tab:twolevel)", "RLCSA", "nojumps.csa"), ("Table 2 (tab:twolevel)", "move structure", "nojumps.move"),
    ("tab:fun", "two-level", "nojumps.move"), ("tab:fun", "almost-funnels, g = 4", "ref4"), ("tab:fun", "almost-funnels, g = 16", "ref16"),
    ("tab:fun", "almost-funnels, g = 64", "ref64"),
    ("tab:heavy", "two-level", "nojumps.move"), ("tab:heavy", "θ = 80%", "heavy08"), ("tab:heavy", "θ = 90%", "heavy09"), ("tab:heavy", "θ = 95%", "heavy095"),
    ("tab:encodings", "two-level", "nojumps.move"),
    ("tab:latency", "two-level", "nojumps.move"), ("tab:latency", "with the table", "nojumps.move+table"),
    ("tab:latency", "with the table and lookahead", "nojumps.move+table+la2"), ("tab:latency", "with lookahead alone", "nojumps.move+la4"),
    ("tab:latency", "heavy paths, θ = 80%", "heavy08"), ("tab:latency", "heavy paths, θ = 80%, with the table", "heavy08+table"),
    ("tab:latency", "heavy paths, θ = 90%", "heavy09"), ("tab:latency", "heavy paths, θ = 90%, with the table", "heavy09+table"),
    ("tab:latency", "heavy paths, θ = 95%", "heavy095"), ("tab:latency", "heavy paths, θ = 95%, with the table", "heavy095+table"),
    ("tab:summary", "two-level (move), plain", "nojumps.move"), ("tab:summary", "table", "nojumps.move+table"),
    ("tab:summary", "table and lookahead", "nojumps.move+table+la2"), ("tab:summary", "lookahead alone", "nojumps.move+la4"),
    ("tab:summary", "STPD of the parse", "stpd"), ("tab:summary", "funnels, g = 64", "ref64"), ("tab:summary", "funnels, g = 16", "ref16"),
    ("tab:summary", "funnels, g = 4", "ref4"), ("tab:summary", "heavy paths, θ = 95%", "heavy095"), ("tab:summary", "heavy paths, θ = 95%, with the table", "heavy095+table"),
    ("tab:summary", "heavy paths, θ = 90%", "heavy09"), ("tab:summary", "heavy paths, θ = 90%, with the table", "heavy09+table"),
    ("tab:summary", "heavy paths, θ = 80%", "heavy08"), ("tab:summary", "heavy paths, θ = 80%, with the table", "heavy08+table")]
print("# Corrected MB for the rows of the posted paper built on the plain `c<N>.lvl` (stage 2, part C0)\n")
print("MB = 10^6 bytes, 16 / 32 / 64 individuals. \"Published\" is recomputed by the paper's rule from the same files and agrees, to within 1 MB of rounding in the first-run tables (which the logs give to 1 kB), "
      "with the paper. \"Corrected\" is the same sum with `c<N>.lvl` replaced by the file without its pairs (a real file, "
      "written by `bin/strip-pairs`; searches on it give byte-identical output), and for rows with the table, without the "
      "pairs' first-run table.\n")
print("| table | row | published | corrected | difference |\n|---|---|---|---|---|")
for t, lab, row in ROWS:
    pub, cor = [], []
    for n in NS:
        p = mb(n, row, f"{D}/c{n}.lvl"); c = mb(n, row, f"{DW}/c{n}np.lvl")
        if "table" in row: a, b = ft(n, row); p += a + b; c += a
        pub.append(p / 1e6); cor.append(c / 1e6)
    print(f"| {t} | {lab} | " + " / ".join(f"{x:,.0f}" for x in pub) + " | " + " / ".join(f"{x:,.0f}" for x in cor) + " | "
          + " / ".join(f"−{x - y:,.0f} ({100 * (x - y) / x:.1f}%)" for x, y in zip(pub, cor)) + " |")
print("\nThe ▷ rows (tab:encodings \"with encoding\", tab:latency and tab:summary's encoding rows) read their pairs: unchanged. "
      "The rows without the parse (RLCSA, the move structure, almost-funnels on the text, ropebwt3, Movi 2, the STPD-index): unchanged.")
