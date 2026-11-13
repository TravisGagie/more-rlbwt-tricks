#!/usr/bin/env python3
"""Summary of experiments/stpd_parse_profile.sh (2026-11-12, item 1): times and the LCE queries' statistics.
   stpd_parse_profile.py N QUERY_PREFIX OUTDIR   -> markdown on stdout"""
import re, sys, statistics as st
import numpy as np

N, QP, O = sys.argv[1], sys.argv[2], sys.argv[3]
L = f"{O}/logs/q{N}"

def total(log):
    return float(re.search(r"\(total ([0-9.]+) s\)", open(log).read()).group(1))

plain = [total(f"{L}.plain.{r}.log") for r in (1, 2, 3)]
prof = [total(f"{L}.prof.{r}.log") for r in (1, 2, 3)]
parts = {}
for r in (1, 2, 3):
    for m in re.finditer(r"profile time: (.+?) ([0-9.]+) s", open(f"{L}.prof.{r}.log").read()):
        parts.setdefault(m.group(1), []).append(float(m.group(2)))
stats = open(f"{L}.prof.1.log").read()
pline = re.search(r"profile: .*", stats).group(0)
sline = re.search(r"STPD over the parse: .*", stats).group(0)

dt = np.dtype([("kind", "u1"), ("dir", "u1"), ("flags", "u1"), ("pad0", "u1"), ("segment", "<u4"), ("search", "<u8"),
               ("p", "<i8"), ("x", "<u8"), ("skip", "<u4"), ("answer", "<u4"), ("limit", "<u4"), ("pad1", "<u4")])
q = np.fromfile(QP + ".queries", dtype=dt)
pr, ex = q[q["kind"] == 0], q[q["kind"] == 1]
nsearch = int(re.search(r"STPD searches (\d+) backward \+ (\d+) forward", pline).group(1)) + \
          int(re.search(r"STPD searches (\d+) backward \+ (\d+) forward", pline).group(2))

def dist(a):
    a = np.asarray(a, dtype=np.float64)
    if len(a) == 0: return "none"
    p = np.percentile(a, [50, 90, 99])
    return f"mean {a.mean():.2f}, median {p[0]:.0f}, 90% {p[1]:.0f}, 99% {p[2]:.0f}, max {a.max():.0f}"

def per_search(rec, w):
    s = np.bincount(rec["search"].astype(np.int64), weights=w, minlength=nsearch + 1)[1:]
    return s

med = lambda v: st.median(v)
print(f"# Profile of our STPD of the parse, c{N} (16 individuals if 33), BML, L = 1000, one occurrence per MEM\n")
print(f"Made by `experiments/stpd_parse_profile.sh {N}`; logs in `logs/`. Times are medians of 3 runs.\n")
print("## Time\n")
print(f"- **Unprofiled (`bin/katka`):** {med(plain):.2f} s ({', '.join(f'{x:.2f}' for x in plain)})")
print(f"- **Profiled (`bin/katka-prof`, TSC timers):** {med(prof):.2f} s ({', '.join(f'{x:.2f}' for x in prof)}); "
      f"the timers add {100 * (med(prof) / med(plain) - 1):.1f}%\n")
print("| part | s | share of the profiled total |\n|---|---:|---:|")
tot = med(prof); acc = 0
for k, v in parts.items():
    if k.startswith("sum of the parts"): continue
    print(f"| {k} | {med(v):.3f} | {100 * med(v) / tot:.1f}% |"); acc += med(v)
print(f"| everything else (reading the reads, BML's bookkeeping, output, ...) | {tot - acc:.3f} | {100 * (tot - acc) / tot:.1f}% |")
print(f"| total | {tot:.3f} | 100% |\n")
print(f"`{sline}`\n\n`{pline}`\n")
print("## LCE queries\n")
print(f"All of them are recorded ({len(q):,}: {len(pr):,} probes, {len(ex):,} extension runs) in "
      f"`{QP}.queries` (48-byte records, `src/prof.hpp`), with the patterns' phrase ids in `{QP}.patterns`.\n")
print(f"- **STPD searches:** {nsearch:,}; with at least one binary search: "
      f"{len(np.unique(pr['search'])):,}; binary searches: {len(np.unique(pr[['search', 'p']])):,}.")
ps = per_search(pr, np.ones(len(pr)))
print(f"- **Probes per search:** {dist(ps)}; per binary search: {dist(np.unique(pr[['search','p']], return_counts=True)[1])}.")
print(f"- **Probes, as forward LCEs** (the pattern's phrases from p against Z from x, left to right; lexicographic sample):")
print(f"  - LCE length (answer): {dist(pr['answer'])};")
print(f"  - phrases compared (answer - skip, plus the mismatch): {dist(pr['answer'].astype(np.int64) - pr['skip'] + 1)};")
print(f"  - skipped (min of the LCPs with both ends): {dist(pr['skip'])};")
print(f"  - answer = limit (the probe goes on to the next phrase's characters): {100 * np.mean(pr['answer'] == pr['limit']):.1f}%.")
print(f"- **Extension, as backward LCEs** (Z[u-1] against the pattern's previous phrase, right to left):")
print(f"  - per run: {dist(ex['answer'])}; ended by a mismatch {100 * np.mean(ex['flags'] == 1):.1f}%;")
pe = per_search(ex, ex["answer"].astype(np.float64) + (ex["flags"] == 1))
print(f"  - phrases read per search (matches, plus each mismatch): {dist(pe)}.")
