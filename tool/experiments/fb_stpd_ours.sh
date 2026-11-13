#!/bin/bash
# Our own STPD over the plain text (katka find --stpd --no-intervals), BML and forward-backward, chr19, in Table 1's
# setup (2026-11-10, item 3: for comparison, not for the paper).  From tool/:
#
#   experiments/fb_stpd_ours.sh [NS] [REPEATS]
#
# NS (default "33 65": 16 and 32 individuals; at 64 the 15.9 GB text doesn't fit in 11 GB).  For each, REPEATS
# (default 3) runs of `find -L 1000 --format mems --stats -j 1 --stpd --no-intervals [--fb]` on the two query
# haplotypes, pinned to CPU $PIN (default 0), in an 11 GB cgroup without swap.  The MEMs (name, start, end) must
# equal the paper's BML output (blocks/q<N>s.blocks.tsv).  Logs in results/chr19/fb/logs/q<N>.stpd-ours.{bml,fb}.<r>.log.
set -euo pipefail
cd "$(dirname "$0")/.."
NS=${1:-"33 65"}; REP=${2:-3}
D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; Q=$D/blocks/queries.fa; O=results/chr19/fb; mkdir -p $O/logs
SCR=$(mktemp -d -p "${TMPDIR:-/tmp}" fb_ours.XXXX)
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"; TS="taskset -c ${PIN:-0}"
for n in $NS; do
  for m in bml fb; do
    opt=""; [ $m = fb ] && opt="--fb"
    for r in $(seq 1 $REP); do
      log=$O/logs/q$n.stpd-ours.$m.$r.log
      echo "[$(date '+%F %T')] c$n our STPD $m repeat $r" >&2
      $CAP $TS bin/katka find -L 1000 --format mems --stats -j 1 --stpd --no-intervals $opt $D/c$n $Q $SCR/out.tsv > $log 2>&1
      cmp -s <(cut -f1-3 $SCR/out.tsv) <(cut -f1-3 $D/blocks/q${n}s.blocks.tsv) || { echo "c$n our STPD $m: MEMs differ from BML's"; exit 1; }
    done
  done
done
rm -rf $SCR
