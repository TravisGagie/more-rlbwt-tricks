#!/usr/bin/env python3
# Latency hiding on the move structure, part 1, steps 1-3: the search's counters (full run minus the 100-character run)
# from p1_stat.sh, and the two bounds on plain search with the move structure, time x (1 - share stalled on an L3 miss)
# and time x (1 - share stalled on any memory access), next to the best two-level row of the final timing session.
#   python3 p1_summary.py P1DIR FINAL_RUNS.tsv
import sys, statistics, collections
W, RUNS = sys.argv[1:3]
IND = {33: 16, 65: 32, 129: 64}; NS = [33, 65, 129]; MODES = {"off": "without huge pages", "hp": "with huge pages"}
def csv(fn):
    d = {}
    for l in open(fn):
        f = l.strip().split(",")
        if len(f) > 4 and f[0].isdigit(): d[f[2].replace(":u", "")] = int(f[0])
    return d
def counts(n, row, a, m):
    s = {}
    for g in (1, 2):
        x, y = csv(f"{W}/q{n}.{row}.{a}.{m}.stat{g}.csv"), csv(f"{W}/q{n}.{row}.{a}.{m}.load{g}.csv")
        for k in x: s[k + (str(g) if k in ("cycles", "instructions") else "")] = x[k] - y[k]
    return s
def shares(s):
    c1, c2 = s["cycles1"], s["cycles2"]
    return {"cyc": c1, "ipc": s["instructions1"] / c1, "tot": s["cycle_activity.stalls_total"] / c1,
            "mem": s["cycle_activity.stalls_mem_any"] / c1, "l3": s["cycle_activity.stalls_l3_miss"] / c1,
            "l3loads": s["mem_load_retired.l3_miss"], "walk": s["dtlb_load_misses.walk_active"] / c2}
t = collections.defaultdict(list)
for l in list(open(RUNS))[1:]:
    f = l.rstrip("\n").split("\t")
    if int(f[4]) <= 3: t[(int(f[0]), f[1], f[2], f[3])].append(float(f[5]))
med = lambda i, row, a, m: statistics.median(t[(i, row, a, m)])
TWO = ["nojumps.move", "nojumps.move+table", "nojumps.move+table+la2", "nojumps.move+la4", "ref4", "ref16", "ref64",
       "heavy08", "heavy08+table", "heavy09", "heavy09+table", "heavy095", "heavy095+table", "unique", "unique+table", "unique+table+la2"]
print("## Counters of the search (user space only; the 100-character run subtracted)\n")
print("| row | individuals | algorithm | mode | cycles | IPC | stalled, any reason | on any memory access | on an L3 miss | L3-miss loads | page walks |")
print("|---|---:|---|---|---:|---:|---:|---:|---:|---:|---:|")
S = {}
for row, rn in (("move", "plain, move structure"), ("heavy09t", "heavy paths θ = 90%, with the first-run table")):
    for n in NS:
        for a in ("bml", "fb"):
            for m in ("off", "hp"):
                x = shares(counts(n, row, a, m)); S[(row, n, a, m)] = x
                print(f"| {rn} | {IND[n]} | {a.upper()} | {MODES[m]} | {x['cyc']:.3e} | {x['ipc']:.2f} | {100 * x['tot']:.1f}% | "
                      f"{100 * x['mem']:.1f}% | {100 * x['l3']:.1f}% | {x['l3loads']:.3e} | {100 * x['walk']:.1f}% |")
print("\n## Bounds on plain search with the move structure\n")
print("Times are the final timing session's medians (`final/runs.tsv`). Bound L3 = time x (1 - share stalled on an L3 miss);"
      " bound mem = time x (1 - share stalled on any memory access); best = the fastest two-level row in the same mode,"
      " algorithm and size; ratio = bound / best.\n")
print("| individuals | algorithm | mode | time | bound L3 | bound mem | best two-level row | best | bound L3 / best | bound mem / best | heavy θ = 90% + table | bound mem / it |")
print("|---:|---|---|---:|---:|---:|---|---:|---:|---:|---:|---:|")
for n in NS:
    i = IND[n]
    for a in ("bml", "fb"):
        for m in ("off", "hp"):
            x = S[("move", n, a, m)]; tm = med(i, "move", a, m)
            bl, bm = tm * (1 - x["l3"]), tm * (1 - x["mem"])
            best = min(TWO, key=lambda r: med(i, r, a, m)); tb = med(i, best, a, m); th = med(i, "heavy09+table", a, m)
            print(f"| {i} | {a.upper()} | {MODES[m]} | {tm:.2f} | {bl:.2f} | {bm:.2f} | {best} | {tb:.2f} | {bl / tb:.2f} | {bm / tb:.2f} | {th:.2f} | {bm / th:.2f} |")
