#!/bin/bash
# Our STPD of the parse with the parse RLZ-compressed against CHM13's parse (2026-11-12, "STPD of the parse
# with RLZ").  From tool/:   experiments/stpd_parse_rlz.sh [NS] [REPEATS]     (NS default 33)
# Configurations, BML, L = 1000, `find --csa --stpd-parse --no-intervals --stats -j 1`:
#   plain    the plain parse (c<N>.ptxt);
#   greedy   --rlzap-oracle on ~/research/rlbwt-tricks/chr19/rlzg/c<N>: the greedy RLZ (index --rlzap=0,0,1000000000:
#            no look-ahead, so the greedy longest match, keeping the pointer among equally long ones; 3835d67's factors);
#   rlzap    --rlzap-oracle on c<N>: the tool's RLZAP oracle (index --rlzap, J = 4, D = 4, M = 2);
#   unique   two-level with the unique-successor (▷) encoding (`--csa --twolevel`, unique/c<N>u).
# REPEATS (default 3) rounds, each running every configuration once, pinned to CPU $PIN (default 0), in 11 GB
# cgroups; MEMs checked against the paper's.  Logs in results/chr19/stpd_parse_rlz/logs/.
set -euo pipefail
cd "$(dirname "$0")/.."
NS=${1:-33}; REP=${2:-3}
D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; Q=$D/blocks/queries.fa; O=results/chr19/stpd_parse_rlz; mkdir -p $O/logs
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"; TS="taskset -c ${PIN:-0}"
SCR=$(mktemp -d -p "${TMPDIR:-$HOME/research/rlbwt-tricks/tmp}" stpd_parse_rlz.XXXX)
for n in $NS; do
  for r in $(seq 1 $REP); do
    for c in plain greedy rlzap unique; do
      case $c in
        plain)  X="--stpd-parse --no-intervals $D/c$n" ;;
        greedy) X="--stpd-parse --no-intervals --rlzap-oracle $D/rlzg/c$n" ;;
        rlzap)  X="--stpd-parse --no-intervals --rlzap-oracle $D/c$n" ;;
        unique) X="--twolevel $D/unique/c${n}u" ;;
      esac
      log=$O/logs/q$n.$c.$r.log
      echo "[$(date '+%F %T')] c$n $c repeat $r" >&2
      /usr/bin/time -f "wall %e s, peak %M KB" $CAP $TS bin/katka find -L 1000 --format mems --csa --stats -j 1 $X $Q $SCR/out.tsv > $log 2>&1
      cmp -s <(cut -f1-3 $SCR/out.tsv) <(cut -f1-3 $D/blocks/q${n}s.blocks.tsv) || { echo "c$n $c: MEMs differ from BML's"; exit 1; }
    done
  done
done
rm -rf $SCR
