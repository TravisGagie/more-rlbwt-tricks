#!/bin/bash
# BML reruns beside the FB runs of 2026-11-11, so both are timed on the same day: plain backward search on the
# RLCSA, both strands (item 1), and the two-level index with no block steps (item 2, CONFIGS nojumps), and
# the shortcut configurations heavy08, heavy095, ref4, ref16, ref64 (items 3, 4); same setup as experiments/fb_timing.sh.  From tool/:
#   experiments/bml_retime.sh [NS] [REPEATS] [CONFIGS (default plain)]
# Logs in results/chr19/fb/logs/q<N>.<config>.bml.<r>.log; MEMs (name, start, end) must equal blocks/q<N>s.blocks.tsv.
set -euo pipefail
cd "$(dirname "$0")/.."
NS=${1:-"33 65 129"}; REP=${2:-3}; CONFIGS=${3:-plain}
D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; Q=$D/blocks/queries.fa; O=results/chr19/fb; mkdir -p $O/logs
SCR=$(mktemp -d -p "${TMPDIR:-/tmp}" plain_bml.XXXX)
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"; TS="taskset -c ${PIN:-0}"
for n in $NS; do
  for c in $CONFIGS; do
    case $c in plain) X="" ;; nojumps) X="--twolevel --no-jumps" ;;
      heavy*|ref*) X="--twolevel --pshortcuts $D/psc/c$n.$c.psc" ;; *) echo "unknown config $c"; exit 1 ;; esac
    for r in $(seq 1 $REP); do
      log=$O/logs/q$n.$c.bml.$r.log
      echo "[$(date '+%F %T')] c$n $c BML repeat $r" >&2
      $CAP $TS bin/katka find -L 1000 --format mems --csa $X --stats -j 1 $D/c$n $Q $SCR/out.tsv > $log 2>&1
      cmp -s <(cut -f1-3 $SCR/out.tsv) <(cut -f1-3 $D/blocks/q${n}s.blocks.tsv) || { echo "c$n $c BML: MEMs differ"; exit 1; }
    done
  done
done
rm -rf $SCR
