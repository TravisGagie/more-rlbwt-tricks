#!/bin/bash
# Profile of the costs shared by the two-level methods (2026-11-12, "answer on the STPD profile", A-C).
# From tool/:   experiments/twolevel_profile.sh [N] [CONFIGS] [ALGS]
# N (default 33), CONFIGS (default "nojumps unique heavy08": two-level without jumps, the unique-successor (▷)
# encoding, heavy-path almost-funnels theta = 0.8), ALGS (default "bml fb").  For each: bin/katka once (unprofiled)
# and bin/katka-prof once (src/prof.hpp timers), `find -L 1000 --format mems --csa --twolevel --stats -j 1`, pinned to
# CPU $PIN (default 0) in an 11 GB cgroup; MEMs checked against the paper's.  Logs in results/chr19/twolevel_profile/logs/.
set -euo pipefail
cd "$(dirname "$0")/.."
N=${1:-33}; CONFIGS=${2:-"nojumps unique heavy08"}; ALGS=${3:-"bml fb"}; BIN=${BIN:-bin/katka-prof}; TAG=${TAG:-prof}
D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; Q=$D/blocks/queries.fa; O=results/chr19/twolevel_profile; mkdir -p $O/logs
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"; TS="taskset -c ${PIN:-0}"
SCR=$(mktemp -d -p "${TMPDIR:-$HOME/research/rlbwt-tricks/tmp}" twolevel_profile.XXXX)
for c in $CONFIGS; do
  case $c in
    blocks)  X="$D/c$N" ;;
    unique)  X="$D/unique/c${N}u" ;;
    heavy*)  X="--pshortcuts $D/psc/c$N.$c.psc $D/c$N" ;;
    nojumps) X="--no-jumps $D/c$N" ;;
  esac
  for a in $ALGS; do
    F=""; [ $a = fb ] && F="--fb"
    for b in bin/katka $BIN; do
      t=plain; [ $b = bin/katka ] || t=$TAG
      log=$O/logs/q$N.$c.$a.$t.log
      echo "[$(date '+%F %T')] c$N $c $a $b" >&2
      $CAP $TS $b find -L 1000 --format mems --csa --twolevel --stats -j 1 $F $X $Q $SCR/out.tsv > $log 2>&1
      cmp -s <(cut -f1-3 $SCR/out.tsv) <(cut -f1-3 $D/blocks/q${N}s.blocks.tsv) || { echo "c$N $c $a: MEMs differ from BML's"; exit 1; }
    done
  done
done
rm -rf $SCR
