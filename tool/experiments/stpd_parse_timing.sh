#!/bin/bash
# STPD over the parse on the two-level index (2026-11-08 evening, item 2), in Table 1's setup: the
# two query haplotypes, L = 1000, BML, one thread pinned to CPU $PIN (default 0), 11 GB cgroup without swap,
# REPEATS (default 3) runs.  `find --csa --stpd-parse --no-intervals` on ~/research/rlbwt-tricks/chr19/c<n> (needs
# c<n>.ptxt, .pstpd, .stpd, .lvl, .csa, .rlbwt; not the text).  With MODES="parse text" (default "parse"), also
# the STPD over the text, `find --stpd --no-intervals` (needs c<n>.S, .stpd, .rlbwt).  The MEMs (name, start,
# end) must equal the paper's blocks/q<n>s.blocks.tsv.  From tool/:  experiments/stpd_parse_timing.sh [NS] [REPEATS]
set -euo pipefail
cd "$(dirname "$0")/.."
NS=${1:-"33 65"}; REP=${2:-3}
D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; Q=$D/blocks/queries.fa; O=${O:-results/chr19/stpd_parse_timing}; mkdir -p $O/logs
SCR=$(mktemp -d -p "${TMPDIR:-/tmp}" stpd_parse_timing.XXXX); mkdir -p $SCR
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"; TS="taskset -c ${PIN:-0}"
for n in $NS; do
  for mode in ${MODES:-parse}; do
    case $mode in
      parse)  need="ptxt pstpd stpd lvl csa rlbwt"; opt="--csa --stpd-parse" ;;
      text)   need="S stpd rlbwt"; opt="--stpd" ;;
      oracle) need="ptxt stpd rlbwt"; opt="--stpd --parse-oracle" ;;   # the character-level STPD, the text read through the parse
    esac
    for f in $need; do [ -s $D/c$n.$f ] || { echo "missing $D/c$n.$f"; exit 1; }; done
    for r in $(seq 1 $REP); do
      log=$O/logs/q$n.stpd-$mode.$r.log
      echo "[$(date '+%F %T')] c$n stpd-$mode repeat $r" >&2
      /usr/bin/time -f "wall %e s, peak %M KB" $CAP $TS bin/katka find -L 1000 --format mems $opt --no-intervals --stats -j 1 $D/c$n $Q $SCR/out.tsv > $log 2>&1
      cmp -s <(cut -f1-3 $SCR/out.tsv) <(cut -f1-3 $D/blocks/q${n}s.blocks.tsv) || { echo "c$n $mode: MEMs differ from the paper's"; exit 1; }
    done
  done
done
rm -rf $SCR
for n in 33 65 129; do for mode in parse text oracle; do
  ls $O/logs/q$n.stpd-$mode.*.log > /dev/null 2>&1 || continue
  ts=$(grep -h "reads=" $O/logs/q$n.stpd-$mode.*.log | sed 's/.*wall \([0-9.]*\) s.*/\1/' | sort -n | tr '\n' ' ')
  echo "c$n stpd-$mode: times $ts(median $(echo $ts | awk '{print $2}')); $(grep -h '^wall' $O/logs/q$n.stpd-$mode.1.log)"
  grep -h "components (MB)\|^STPD:" $O/logs/q$n.stpd-$mode.1.log
done; done | tee $O/summary.txt
