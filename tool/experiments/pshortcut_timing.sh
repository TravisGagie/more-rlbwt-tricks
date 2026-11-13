#!/bin/bash
# Shortcuts over phrases on the two-level index, chr19 (2026-11-08 evening, item 3), in Table 1's
# setup.  From tool/:
#
#   experiments/pshortcut_timing.sh N "TAGS" [REPEATS]
#
# N: 33, 65 or 129 (16, 32, 64 individuals).  TAGS: shortcut files ~/research/rlbwt-tricks/chr19/psc/c<N>.<tag>.psc,
# built by `katka index --pshortcuts SPEC --pshortcuts-out FILE ~/research/rlbwt-tricks/chr19/c<N>` (e.g. tag heavy09
# for heavy:0.9, ref16 for ref:16).  For each, REPEATS (default 3) runs of `find -L 1000 --format mems --csa
# --twolevel --stats -j 1 --pshortcuts FILE` on the two query haplotypes, pinned to CPU $PIN (default 0), in an
# 11 GB cgroup without swap.  Every output (MEMs and BWT intervals) must equal the paper's blocks/q<N>s.blocks.tsv,
# which equals plain BML's.  Logs to results/chr19/pshortcuts/logs/; the summary is made by pshortcut_table.py.
set -euo pipefail
cd "$(dirname "$0")/.."
n=$1; TAGS=$2; REP=${3:-3}
D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; Q=$D/blocks/queries.fa; O=results/chr19/pshortcuts; mkdir -p $O/logs
SCR=${TMPDIR:-/tmp}/pshortcut_timing.$$; mkdir -p $SCR
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"; TS="taskset -c ${PIN:-0}"
for t in $TAGS; do
  F=$D/psc/c$n.$t.psc; [ -s $F ] || { echo "missing $F"; exit 1; }
  for r in $(seq 1 $REP); do
    log=$O/logs/q$n.$t.$r.log
    echo "[$(date '+%F %T')] c$n $t repeat $r" >&2
    $CAP $TS bin/katka find -L 1000 --format mems --csa --twolevel --stats -j 1 --pshortcuts $F $D/c$n $Q $SCR/out.tsv > $log 2>&1
    cmp -s $SCR/out.tsv $D/blocks/q${n}s.blocks.tsv || { echo "c$n $t: MEMs differ from the paper's"; exit 1; }
  done
done
rm -rf $SCR
