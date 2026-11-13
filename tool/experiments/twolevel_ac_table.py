#!/usr/bin/env python3
"""Table of experiments/twolevel_ac.sh (2026-11-12, A-C and the remaining reruns): medians of 3 (s) and the MB loaded.
   twolevel_ac_table.py [OUTDIR [CONFIG ...]]"""
import sys
import os, re, statistics, glob

D = os.environ.get("CHR19", os.path.expanduser("~/research/rlbwt-tricks/chr19"))
O = sys.argv[1] if len(sys.argv) > 1 else "results/chr19/twolevel_ac"
NS = [int(x) for x in os.environ.get("SIZES", "33 65 129").split()]
CONFIGS = sys.argv[2:] or ["rlcsa", "nojumps.csa", "nojumps.move", "unique.csa", "unique.move", "heavy08.csa", "heavy08.move"]
NAME = {"rlcsa": "plain backward search, RLCSA", "nojumps.csa": "two-level, no jumps; RLCSA", "nojumps.move": "two-level, no jumps; move structure",
        "unique.csa": "two-level, ▷; RLCSA", "unique.move": "two-level, ▷; move structure",
        "heavy08.csa": "two-level, heavy paths θ = 0.8; RLCSA", "heavy08.move": "two-level, heavy paths θ = 0.8; move structure",
        "rlcsa.move": "plain backward search, move structure alone", "stpd": "our STPD of the parse (plain parse), one occurrence per MEM; RLCSA"}
for th, nm in (("heavy09", "0.9"), ("heavy095", "0.95")):
    for t, tn in (("csa", "RLCSA"), ("move", "move structure")): NAME[f"{th}.{t}"] = f"two-level, heavy paths θ = {nm}; {tn}"
for g in (4, 16, 64):
    for t, tn in (("csa", "RLCSA"), ("move", "move structure")): NAME[f"ref{g}.{t}"] = f"two-level, almost-funnels from CHM13's parse, g = {g}; {tn}"
for e, t, nm in (("25", "0", "fixed-length, windows of 25 (τ = 0)"), ("100", "0", "fixed-length, windows of 100 (τ = 0)"), ("25", "6", "variable-length, windows of 25, τ = 6")):
    NAME[f"tsc{e}t{t}"] = f"plain backward search, move structure, almost-funnels on the text (both strands), {nm}"
TABLE8 = 65536 * 32  # the 8-mer cursor table built at load with --move

def size(p):
    return os.stat(p).st_size

def loaded(n, c):
    base, t = (c.split(".") + ["csa"])[:2]
    if base == "rlcsa":
        return size(f"{D}/c{n}.csa") if t == "csa" else size(f"{D}/c{n}.mv") + TABLE8
    if base.startswith("tsc"):
        return size(f"{D}/c{n}.mv") + TABLE8 + size(f"{D}/tsc/c{n}.ell{base[3:]}.tsc")
    if base == "stpd":       # the files the search opens (results/chr19/stpd_parse_fb/opened.q<N>.txt)
        return sum(size(f"{D}/c{n}.{e}") for e in ("csa", "lvl", "pstpd", "ptxt", "rlbwt", "stpd"))
    b = size(f"{D}/c{n}.rlbwt")
    b += size(f"{D}/c{n}.csa") if t == "csa" else size(f"{D}/c{n}.mv") + TABLE8
    b += size(f"{D}/unique/c{n}u.lvl") if base == "unique" else size(f"{D}/c{n}.lvl")
    if base.startswith("heavy") or base.startswith("ref"):
        b += size(f"{D}/psc/c{n}.{base}.psc")
    return b

def med(n, c, a):
    v = []
    for f in sorted(glob.glob(f"{O}/logs/q{n}.{c}.{a}.*.log")):
        m = re.search(r"\(total ([0-9.]+) s\)", open(f).read())
        if m: v.append(float(m.group(1)))
    return (statistics.median(v), len(v)) if v else (None, 0)

print("| configuration | " + " | ".join(f"{(n - 1) // 2}: BML / FB (s) | MB" for n in NS) + " |")
print("|---|" + "---:|---:|" * len(NS))
for c in CONFIGS:
    row = [NAME[c]]
    for n in NS:
        b, nb = med(n, c, "bml"); f, nf = med(n, c, "fb")
        row.append("–" if b is None else f"{b:.2f} / {f:.2f}" + ("" if nb == 3 and nf == 3 else f" ({nb}, {nf} runs)"))
        row.append(f"{loaded(n, c) / 1e6:,.1f}")
    print("| " + " | ".join(row) + " |")
