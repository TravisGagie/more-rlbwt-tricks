#!/usr/bin/env python3
# Summary of B (latency study): counters of the search (full run minus the 100-character run), shares of cycle samples
# and of L3-miss loads by class, and X = (fraction of cycles stalled on an L3 miss) x (share of L3-miss loads in the
# parse's index), split over the classes.
#   python3 b_summary.py W "q33.nojumps q33.unique ..."
import sys, re
W, ROWS = sys.argv[1], sys.argv[2].split()
def csv(fn):
    d = {}
    for l in open(fn):
        f = l.strip().split(",")
        if len(f) > 4 and f[0].isdigit(): d[f[2].replace(":u", "")] = int(f[0])
    return d
def cls(fn):
    tot, sub = {}, {}
    sec = 0
    for l in open(fn):
        if l.startswith("parse's index by class"): sec = 1; continue
        if l.startswith("parse's index, top"): sec = 2; continue
        m = re.match(r"\s+([\d.]+)%\s+(.*)$", l)
        if not m: continue
        if sec == 0: tot[m.group(2).strip()] = float(m.group(1))
        elif sec == 1: sub[m.group(2).strip()] = float(m.group(1))
    return tot, sub
for r in ROWS:
    s = {}; ld = {}
    for g in (1, 2):
        a, b = csv(f"{W}/{r}.stat{g}.csv"), csv(f"{W}/{r}.load{g}.csv")
        for k in a:
            if k in ("cycles", "instructions"): s[k + str(g)] = a[k] - b[k]
            else: s[k] = a[k] - b[k]
    c1, c2 = s["cycles1"], s["cycles2"]
    wall = re.search(r"wall ([0-9.]+) s", open(f"{W}/{r}.stat1.log").read()).group(1)
    l3f = s["cycle_activity.stalls_l3_miss"] / c1
    print(f"== {r}: search wall {wall} s (stat run); cycles {c1:.3e} / {c2:.3e}; IPC {s['instructions1'] / c1:.2f}")
    print(f"   stalled on an L3 miss {100 * l3f:.1f}% of cycles; on any memory access {100 * s['cycle_activity.stalls_mem_any'] / c1:.1f}%; "
          f"for any reason {100 * s['cycle_activity.stalls_total'] / c1:.1f}%; page walks {100 * s['dtlb_load_misses.walk_active'] / c2:.1f}%; "
          f"L3-miss loads {s['mem_load_retired.l3_miss']:.3e}; branch misses per 1000 instructions {1000 * s['branch-misses'] / s['instructions2']:.2f}")
    ct, cs = cls(f"{W}/{r}.cyc.class.txt"); lt, ls = cls(f"{W}/{r}.l3.class.txt")
    keys = sorted(set(ct) | set(lt), key=lambda k: -(ct.get(k, 0) + lt.get(k, 0)))
    print("   class: cycle samples % / L3-miss loads %")
    for k in keys:
        if ct.get(k, 0) + lt.get(k, 0) >= 0.5: print(f"     {ct.get(k, 0):6.2f} / {lt.get(k, 0):6.2f}  {k}")
    pi = sum(v for k, v in lt.items() if k.startswith("parse's index"))
    print(f"   X = {100 * l3f:.1f}% x {pi:.2f}% = {l3f * pi:.2f}% of cycles")
    for k in sorted(ls): print(f"     X part {l3f * ls[k]:5.2f}% of cycles (L3-miss loads {ls[k]:5.2f}%; cycle samples {cs.get(k, 0):5.2f}%)  {k}")
