#!/bin/bash
# Forward-backward search (Li's: forward from each start, then backward from the match's end) on the two-level
# index, chr19, in Table 1's setup (2026-11-10, item 1).  From tool/:
#
#   experiments/fb_timing.sh [NS] [CONFIGS] [REPEATS]
#
# NS (default "33 65 129": 16, 32, 64 individuals).  CONFIGS (default "blocks unique heavy09"):
#   blocks   the paper's index, smallest-successor blocks (~/research/rlbwt-tricks/chr19/c<N>);
#   unique   unitig jumps, blocks by the unitig rule (unique/c<N>u, results/chr19/unique/);
#   heavy09  heavy-path shortcuts, theta = 0.9 (psc/c<N>.heavy09.psc on c<N>; results/chr19/pshortcuts/); likewise
#            heavy08, heavy095 (theta = 0.8, 0.95) and ref4, ref16, ref64 (shortcuts from CHM13's parse, g = 4, 16, 64);
#   nojumps  the two-level index with no block steps (c<N>, --no-jumps; 2026-11-11, item 2);
#   plain    plain backward search on the RLCSA of the text, both strands (c<N>, no --twolevel; 2026-11-11, item 1).
# Each: REPEATS (default 3) runs of `find -L 1000 --format mems --csa --twolevel --stats -j 1 --fb` on the two query
# haplotypes, pinned to CPU $PIN (default 0), in an 11 GB cgroup without swap.  The MEMs (name, start, end) must be
# those of the paper's BML output blocks/q<N>s.blocks.tsv.  Logs in results/chr19/fb/logs/; table by fb_table.py.
set -euo pipefail
cd "$(dirname "$0")/.."
NS=${1:-"33 65 129"}; CONFIGS=${2:-"blocks unique heavy09"}; REP=${3:-3}
D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; Q=$D/blocks/queries.fa; O=results/chr19/fb; mkdir -p $O/logs
SCR=$(mktemp -d -p "${TMPDIR:-/tmp}" fb_timing.XXXX)
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"; TS="taskset -c ${PIN:-0}"
for n in $NS; do
  for c in $CONFIGS; do
    TL="--twolevel"
    case $c in
      blocks)  X="$D/c$n" ;;
      unique)  X="$D/unique/c${n}u" ;;
      heavy*|ref*) X="--pshortcuts $D/psc/c$n.$c.psc $D/c$n" ;;
      nojumps) X="--no-jumps $D/c$n" ;;
      plain)   X="$D/c$n"; TL="" ;;
      *) echo "unknown config $c"; exit 1 ;;
    esac
    for r in $(seq 1 $REP); do
      log=$O/logs/q$n.$c.fb.$r.log
      echo "[$(date '+%F %T')] c$n $c FB repeat $r" >&2
      $CAP $TS bin/katka find -L 1000 --format mems --csa $TL --stats -j 1 --fb $X $Q $SCR/out.tsv > $log 2>&1
      cmp -s <(cut -f1-3 $SCR/out.tsv) <(cut -f1-3 $D/blocks/q${n}s.blocks.tsv) || { echo "c$n $c: FB MEMs differ from BML's"; exit 1; }
    done
  done
done
rm -rf $SCR
