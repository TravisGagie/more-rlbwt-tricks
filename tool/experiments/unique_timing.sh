#!/bin/bash
# Unitig blocks (--continuation unique) against the paper's smallest successor on chr19 (2026-11-08
# evening, item 1b).  From tool/:
#
#   experiments/unique_timing.sh [NS] [REPEATS]
#
# NS (default "33 65 129"): the collections of More RLBWT Tricks' Table 1 (16, 32, 64 individuals).  The smallest
# successor's index is the paper's, ~/research/rlbwt-tricks/chr19/c<n>; the unique one is
# ~/research/rlbwt-tricks/chr19/unique/c<n>u, built by `katka index --twolevel --continuation unique --blocks-from
# c<n>.lvl` (the same parse, B and parse BWT; the RLBWT and RLCSA are links).  For each, REPEATS (default 3) runs of
# `find -L 1000 --format mems --csa --twolevel --stats -j 1` on the two query haplotypes, as in Table 1, and of the
# same with --no-jumps (the two-level index without block steps), pinned to CPU $PIN (default 0), under an 11 GB
# memory cap with no swap.  Every output must equal the paper's blocks/q<n>s.blocks.tsv (MEMs and BWT intervals,
# equal to plain BML's).  Writes results/chr19/unique/{runs.tsv,table.md} and the logs.
set -euo pipefail
cd "$(dirname "$0")/.."
NS=${1:-"33 65 129"}; REP=${2:-3}
D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; Q=$D/blocks/queries.fa; O=results/chr19/unique; mkdir -p $O/logs
SCR=${TMPDIR:-/tmp}/unique_timing.$$; mkdir -p $SCR
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"
TS="taskset -c ${PIN:-0}"
for n in $NS; do
  for v in s u; do
    X=$D/c$n; [ $v = u ] && X=$D/unique/c${n}u
    for mode in blocks nojumps; do
      [ $v = u ] && [ $mode = nojumps ] && continue     # without block steps the rule changes nothing
      opt=""; [ $mode = nojumps ] && opt="--no-jumps"
      for r in $(seq 1 $REP); do
        log=$O/logs/q$n$v.$mode.$r.log
        echo "[$(date '+%F %T')] c$n $v $mode repeat $r" >&2
        $CAP $TS bin/katka find -L 1000 --format mems --csa --twolevel $opt --stats -j 1 $X $Q $SCR/out.tsv > $log 2>&1
        cmp -s $SCR/out.tsv $D/blocks/q${n}s.blocks.tsv || { echo "c$n $v $mode: MEMs differ from the paper's"; exit 1; }
      done
    done
  done
done
rm -rf $SCR
python3 - $O "$NS" $REP $D <<'PY'
import os, re, sys, statistics
O, NS, REP, D = sys.argv[1], sys.argv[2].split(), int(sys.argv[3]), sys.argv[4]
QLEN = 122434624
def grab(f, rx):
    m = re.search(rx, open(f).read()); return tuple(int(x) if x.isdigit() else float(x) for x in m.groups())
rows = []
for n in NS:
    for v, mode in (("s", "nojumps"), ("s", "blocks"), ("u", "blocks")):
        logs = [os.path.join(O, "logs", "q%s%s.%s.%d.log" % (n, v, mode, r)) for r in range(1, REP + 1)]
        ts = [grab(f, r"wall ([\d.]+) s,")[0] for f in logs]
        f = logs[0]
        mems = grab(f, r"MEMs=(\d+)")[0]
        c, p, b = grab(f, r"char_steps=(\d+) parse_steps=(\d+) block_steps=(\d+)")
        pe = grab(f, r"parse steps: entry=(\d+) inside=(\d+) end=(\d+); block attempts=(\d+), symbol absent=(\d+), "
                     r"failed block steps=(\d+), back-ups=(\d+), exact exits=(\d+)") if mode == "blocks" else (0,) * 8
        lvl = os.path.join(D, "c%s.lvl" % n) if v == "s" else os.path.join(D, "unique", "c%su.lvl" % n)
        rows.append(dict(n=n, index={"s": "smallest", "u": "unique"}[v] + (" (no block steps)" if mode == "nojumps" else ""),
                         mb=os.path.getsize(lvl) / 1e6, med=statistics.median(ts), ts=ts, mems=mems, char=c, parse=p, block=b,
                         entry=pe[0], inside=pe[1], end=pe[2], attempts=pe[3], nosym=pe[4], bfail=pe[5], backups=pe[6], exact=pe[7]))
keys = ["n", "index", "mb", "med", "ts", "mems", "char", "parse", "block", "entry", "inside", "end", "attempts", "nosym", "bfail", "backups", "exact"]
with open(os.path.join(O, "runs.tsv"), "w") as o:
    o.write("\t".join(keys) + "\n")
    for r in rows: o.write("\t".join(",".join("%.2f" % t for t in r[k]) if k == "ts" else str(r[k]) for k in keys) + "\n")
ind = {"33": 16, "65": 32, "129": 64}
out = ["| individuals | index | .lvl MB | time s (median of %d) | runs | char steps | parse steps | block steps | entry | inside | end | block attempts | symbol absent | failed | back-ups | exact exits |" % REP,
       "|---|---|---:|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|"]
for r in rows:
    out.append("| %d | %s | %.1f | %.2f | %s | %s |" % (ind.get(r["n"], r["n"]), r["index"], r["mb"], r["med"], " ".join("%.2f" % t for t in r["ts"]),
               " | ".join("{:,}".format(r[k]) for k in ["char", "parse", "block", "entry", "inside", "end", "attempts", "nosym", "bfail", "backups", "exact"])))
open(os.path.join(O, "table.md"), "w").write("\n".join(out) + "\n")
print("\n".join(out))
PY
