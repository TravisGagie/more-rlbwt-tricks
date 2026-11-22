#!/bin/bash
# Latency hiding on the move structure, part 1, steps 1 and 3: perf stat on plain search on the move structure (row
# "move": find --move c<N>) and on heavy paths theta = 0.9 with the first-run table (row "heavy09t": bin/katka-table
# --move --twolevel --pshortcuts), BML and FB, without and with huge pages (GLIBC_TUNABLES=glibc.malloc.hugetlb=1);
# and row "co<K>", part 2's plain search chopped into 64 pieces interleaved in K coroutines (bin/katka-chop).
#   experiments/latency/p1_stat.sh "ROWS" "NS"
# As in B (b_perf.sh): two groups of 3 events beside the fixed cycles and instructions (no multiplexing; 3 programmable
# counters are free), each on the queries and on a 100-character query (index load and little else); the search's
# counts are the difference.  perf_event_paranoid 2: user space only (the events count as :u).  One thread pinned to
# CPU $PIN (default 0), 11 GB cgroup without swap.  Every output on the queries must equal the paper's MEMs and
# intervals.  Output in $W (default ~/data/latency/p1): q<N>.<row>.<alg>.<off|hp>.{stat,load}<g>.{csv,log}.
set -euo pipefail
cd "$(dirname "$0")/../.."
ROWS=$1; NS=$2; D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; Q=$D/blocks/queries.fa; W=${W:-$HOME/data/latency/p1}; mkdir -p $W
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"; TS="taskset -c ${PIN:-0}"
G1=cycle_activity.stalls_total,cycle_activity.stalls_mem_any,cycle_activity.stalls_l3_miss
G2=mem_load_retired.l3_miss,dtlb_load_misses.walk_active,branch-misses
[ -s $W/q100.fa ] || { echo ">q100" > $W/q100.fa; awk 'NR==2' $Q | cut -c1-100 >> $W/q100.fa; }
for n in $NS; do for c in $ROWS; do for a in bml fb; do for m in off hp; do
  case $c in
    move) B=bin/katka-base; X="--move $D/c$n" ;;
    heavy09t) B=bin/katka-table; X="--move --twolevel --pshortcuts $D/psc/c$n.heavy09.psc $D/c$n" ;;
    co*) B=bin/katka-chop; X="--move $D/c$n" ;;     # part 2: the 64 pieces interleaved in K coroutines
  esac
  E="X=0"; [ $m = hp ] && E="GLIBC_TUNABLES=glibc.malloc.hugetlb=1"; F=""; [ $a = fb ] && F="--fb"
  case $c in co*) E="$E KATKA_CHOP=64 KATKA_CO_K=${c#co}" ;; esac
  o=$W/q$n.$c.$a.$m; R="find -L 1000 --format mems --stats -j 1 $F $X"
  for g in 1 2; do eval G=\$G$g
    $CAP $TS env $E perf stat -x, -o $o.stat$g.csv -e "cycles,instructions,{$G}" -- $B $R $Q $W/out.tsv > $o.stat$g.log 2>&1
    cmp -s <(cut -f1-5 $W/out.tsv) <(cut -f1-5 $D/blocks/q${n}s.blocks.tsv) || { echo "== STOPPED: $o stat$g differs"; exit 1; }
    $CAP $TS env $E perf stat -x, -o $o.load$g.csv -e "cycles,instructions,{$G}" -- $B $R $W/q100.fa $W/out100.tsv > $o.load$g.log 2>&1
  done
  echo "$o: done; search $(grep -o '(total [0-9.]* s)' $o.stat1.log)"
done; done; done; done
rm -f $W/out.tsv $W/out100.tsv
