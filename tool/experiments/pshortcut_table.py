#!/usr/bin/env python3
"""Table of shortcuts over phrases on chr19 (experiments/pshortcut_timing.sh).

    pshortcut_table.py N [N ...]   (from tool/; writes results/chr19/pshortcuts/table.md and runs.tsv)

For each size: the two-level index without block steps and with blocks (results/chr19/unique/runs.tsv), and every
shortcut file with logs in results/chr19/pshortcuts/logs/ (its build log beside it, ~/research/rlbwt-tricks/chr19/psc/).
"""
import glob, os, re, statistics, sys

O = "results/chr19/pshortcuts"
D = os.path.expanduser("~/research/rlbwt-tricks/chr19")
IND = {"33": 16, "65": 32, "129": 64}
ORDER = ["heavy08", "heavy09", "heavy095", "ref4", "ref16", "ref64"]


def grab(text, rx):
    m = re.search(rx, text)
    return tuple(float(x) if "." in x else int(x) for x in m.groups()) if m else None


rows = []
for n in sys.argv[1:]:
    lvl = os.path.getsize(os.path.join(D, "c%s.lvl" % n)) / 1e6
    for line in open("results/chr19/unique/runs.tsv"):
        f = line.rstrip("\n").split("\t")
        if f[0] == n and f[1] in ("smallest (no block steps)", "smallest"):
            rows.append(dict(n=n, cfg={"smallest": "blocks (smallest successor)"}.get(f[1], "no block steps"), lvl=lvl, psc=0.0,
                             med=float(f[3]), ts=f[4].replace(",", " "), parse=int(f[7]), block=int(f[8]), nsc=0, kavg=0.0,
                             met=0, mism=0, taken=0, crossed=0, failed=0))
    tags = sorted({re.match(r"q%s\.(.+)\.\d+\.log" % n, os.path.basename(p)).group(1) for p in glob.glob(os.path.join(O, "logs", "q%s.*.log" % n))},
                  key=lambda t: ORDER.index(t) if t in ORDER else 99)
    for t in tags:
        logs = sorted(glob.glob(os.path.join(O, "logs", "q%s.%s.*.log" % (n, t))))
        ts = [grab(open(p).read(), r"wall ([\d.]+) s,")[0] for p in logs]
        first = open(logs[0]).read()
        b = open(os.path.join(D, "psc", "build%s.%s.log" % (n, t))).read()
        nsc, kavg = grab(b, r": (\d+) shortcuts \(of \d+ phrases\), ([\d.]+) phrases each")
        mb = grab(first, r"shortcuts over phrases \(.*\): \d+, ([\d.]+) MB")[0]
        parse, block = grab(first, r"parse_steps=(\d+) block_steps=(\d+)")
        met, mism, taken, crossed, failed = grab(first, r"entrances met=(\d+), pattern not alpha=(\d+), taken=(\d+) \(phrases crossed (\d+)\), failed midway=(\d+)")
        rows.append(dict(n=n, cfg="shortcuts " + t, lvl=lvl, psc=mb, med=statistics.median(ts), ts=" ".join("%.2f" % x for x in ts),
                         parse=parse, block=block, nsc=nsc, kavg=kavg, met=met, mism=mism, taken=taken, crossed=crossed, failed=failed))

keys = ["n", "cfg", "lvl", "psc", "med", "ts", "parse", "block", "nsc", "kavg", "met", "mism", "taken", "crossed", "failed"]
with open(os.path.join(O, "runs.tsv"), "w") as o:
    o.write("\t".join(keys) + "\n")
    for r in rows:
        o.write("\t".join(str(r[k]) for k in keys) + "\n")
out = ["| individuals | configuration | MB (.lvl + shortcuts) | time s (median of 3) | runs | parse steps | block steps | shortcuts | phrases each | entrances met | not alpha | taken | phrases crossed | failed midway |",
       "|---|---|---:|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|"]
for r in rows:
    sc = r["nsc"] > 0
    out.append("| %d | %s | %.1f%s | %.2f | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s |" % (
        IND[r["n"]], r["cfg"], r["lvl"] + r["psc"], " (%.1f + %.1f)" % (r["lvl"], r["psc"]) if sc else "", r["med"], r["ts"],
        "{:,}".format(r["parse"]), "{:,}".format(r["block"]),
        *(("{:,}".format(r["nsc"]), "%.2f" % r["kavg"], "{:,}".format(r["met"]), "{:,}".format(r["mism"]), "{:,}".format(r["taken"]),
           "{:,}".format(r["crossed"]), "{:,}".format(r["failed"])) if sc else ("",) * 7)))
open(os.path.join(O, "table.md"), "w").write("\n".join(out) + "\n")
print("\n".join(out))
