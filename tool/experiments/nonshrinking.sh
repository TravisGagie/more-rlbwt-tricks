#!/bin/bash
# How many backward steps could skipping save? (2026-11-11; counting only, untimed.)  From tool/:
#   experiments/nonshrinking.sh [SIZES]       (default "16 32 64" individuals)
# Builds bin/count-nonshrinking and bin/katka-ns (experiments/build_count_nonshrinking.sh), then for each size:
#   rs.<size>.txt   BML (L = 1000) on Reportable Shortcuts' RLBWTs of the text and of the reversed collection (the
#                   index the almost-funnels use), every plain backward step's |I(X)| -> |I(cX)| (items 1-3), and the
#                   same BML with ell = 25 almost-funnels, tau = 0 and tau = 6 (item 4), MEMs checked against plain;
#   parse.<size>.<cfg>.txt   katka's two-level BML (L = 1000, --csa) with --stats, cfg = nojumps (every parse step's
#                   interval sizes, item 5), blocks (smallest successor) and heavy09 (heavy-path shortcuts, theta = 0.9),
#                   MEMs checked against the paper's blocks/q<N>s.blocks.tsv.
# Each in an 11 GB cgroup without swap, pinned to CPU $PIN (default 0).  Output in results/chr19/nonshrinking/.
set -euo pipefail
cd "$(dirname "$0")/.."
SIZES=${1:-"16 32 64"}
experiments/build_count_nonshrinking.sh
H=~/research/rlbwt-tricks/shortcuts/chr19_scaling/hprc_chr19; Q=~/research/rlbwt-tricks/chr19/blocks/queries.txt
D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; QF=$D/blocks/queries.fa
O=results/chr19/nonshrinking; mkdir -p $O
SCR=$(mktemp -d -p "${TMPDIR:-$HOME/research/rlbwt-tricks/tmp}" nonshrinking.XXXX)
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"; TS="taskset -c ${PIN:-0}"
for s in $SIZES; do
  n=$((2 * s + 1)); X=$H/n$s
  echo "[$(date '+%F %T')] $s individuals: RLFM-index steps" >&2
  /usr/bin/time -f "wall %e s, peak %M KB" $CAP $TS bin/count-nonshrinking $X/fwd_t6 $X/rev_t6 $Q 1000 \
    tau0=$X/fwd_t0/ell25_theta0.shortcuts,$X/rev_t0/ell25_theta0.shortcuts \
    tau6=$X/fwd_t6/ell25_theta0.shortcuts,$X/rev_t6/ell25_theta0.shortcuts > $O/rs.$s.txt 2>&1
  grep -q "with shortcuts tau0: 0 patterns" $O/rs.$s.txt && grep -q "with shortcuts tau6: 0 patterns" $O/rs.$s.txt \
    || { echo "$s: shortcut MEMs differ from plain search's (or the run failed)"; exit 1; }
  for c in nojumps blocks heavy09; do
    case $c in nojumps) X2="--no-jumps $D/c$n" ;; blocks) X2="$D/c$n" ;; heavy09) X2="--pshortcuts $D/psc/c$n.heavy09.psc $D/c$n" ;; esac
    echo "[$(date '+%F %T')] $s individuals: parse steps, $c" >&2
    $CAP $TS bin/katka-ns find -L 1000 --format mems --csa --twolevel --stats -j 1 $X2 $QF $SCR/out.tsv > $O/parse.$s.$c.txt 2>&1
    cmp -s <(cut -f1-3 $SCR/out.tsv) <(cut -f1-3 $D/blocks/q${n}s.blocks.tsv) || { echo "$s $c: MEMs differ from BML's"; exit 1; }
  done
done
rm -rf $SCR
