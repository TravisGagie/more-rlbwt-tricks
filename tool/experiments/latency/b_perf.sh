#!/bin/bash
# B of the latency study: perf counters and profiles of katka find (BML, L = 1000, move structure, one thread pinned to
# CPU $PIN, 11 GB cgroup).  bin/katka-g: the same flags plus -g (its .text is byte-identical to bin/katka's).
#   experiments/latency/b_perf.sh "ROWS" "NS" [TAG]      ROWS from nojumps unique heavy09 rlcsa; NS from 33 65 129
# For each row and size:
#   - perf stat, two groups of 3 events beside the fixed cycles and instructions (no multiplexing), on the queries
#     and on a 100-character query (index load and little else); the search's counts are the difference;
#   - perf record -e cycles:u -c 200003, and -e mem_load_retired.l3_miss:upp -c 503 (PEBS: the load itself), both with
#     LBR call stacks (--call-graph lbr --user-callchains: at perf_event_paranoid 2 only user call chains are allowed),
#     classified by experiments/latency/b_classify.py against a copy of the sources in $W/src$TAG.
# Every run on the queries is checked against the paper's MEMs and intervals.  Output in $W (default ~/data/latency/b).
set -euo pipefail
cd "$(dirname "$0")/../.."
ROWS=$1; NS=$2; TAG=${3:-}; D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; Q=$D/blocks/queries.fa; W=${W:-$HOME/data/latency/b}; mkdir -p $W
BIN=${BIN:-bin/katka-g}
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"; TS="taskset -c ${PIN:-0}"
G1=cycle_activity.stalls_total,cycle_activity.stalls_mem_any,cycle_activity.stalls_l3_miss
G2=mem_load_retired.l3_miss,dtlb_load_misses.walk_active,branch-misses
mkdir -p $W/src$TAG && cp src/*.hpp src/*.cpp $W/src$TAG/
[ -s $W/q100.fa ] || { echo ">q100" > $W/q100.fa; awk 'NR==2' $Q | cut -c1-100 >> $W/q100.fa; }
for n in $NS; do for c in $ROWS; do
  case $c in
    nojumps) X="--twolevel --no-jumps $D/c$n" ;;
    unique)  X="--twolevel $D/unique/c${n}u" ;;
    heavy09) X="--twolevel --pshortcuts $D/psc/c$n.heavy09.psc $D/c$n" ;;
    rlcsa)   X="$D/c$n" ;;
  esac
  o=$W/q$n.$c$TAG; F="find -L 1000 --format mems --move --stats -j 1 $X"
  chk() { cmp -s <(cut -f1-5 $W/out.tsv) <(cut -f1-5 $D/blocks/q${n}s.blocks.tsv) || { echo "== STOPPED: $o $1 differs"; exit 1; }; }
  for g in 1 2; do eval G=\$G$g
    $CAP $TS perf stat -x, -o $o.stat$g.csv -e "cycles,instructions,{$G}" -- $BIN $F $Q $W/out.tsv > $o.stat$g.log 2>&1; chk stat$g
    $CAP $TS perf stat -x, -o $o.load$g.csv -e "cycles,instructions,{$G}" -- $BIN $F $W/q100.fa $W/out100.tsv > $o.load$g.log 2>&1
  done
  $CAP $TS perf record -q --call-graph lbr --user-callchains -e cycles:u -c 200003 -o $o.cyc.data -- $BIN $F $Q $W/out.tsv > $o.cyc.log 2>&1; chk cyc
  $CAP $TS perf record -q --call-graph lbr --user-callchains -e mem_load_retired.l3_miss:upp -c 503 -o $o.l3.data -- $BIN $F $Q $W/out.tsv > $o.l3.log 2>&1; chk l3
  echo "$o: done, all outputs equal the paper's; search wall $(grep -o 'wall [0-9.]* s' $o.stat1.log | head -1)"
done; done
rm -f $W/out.tsv $W/out100.tsv
