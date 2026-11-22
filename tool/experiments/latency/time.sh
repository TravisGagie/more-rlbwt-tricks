#!/bin/bash
# Timings of the latency study by the paper's rules (as experiments/twolevel_ac.sh): find -L 1000 --format mems
# --move --stats -j 1, BML and FB, the two query haplotypes, one thread pinned to CPU $PIN (default 0), 11 GB cgroup
# without swap, REP (default 3) interleaved rounds; the time is find's "wall" (load excluded).
#   experiments/latency/time.sh NAME "NS" "ROWS" "VARIANTS" [REP]
# ROWS from nojumps unique heavy09 move (plain backward search on the move structure alone).  VARIANTS: words LABEL=BINARY[:ENV=VALUE[:ENV=VALUE]], e.g.
#   current=bin/katka-base  table=bin/katka  lookahead-alone=bin/katka-la:KATKA_NO_FIRST_RUN=1:KATKA_LA_D=8
# Every output file must be byte-identical to bin/katka-base's for the same row, size and algorithm (made once, in
# $W/ref).  Logs in $W/NAME/q<N>.<row>.<label>.<alg>.<r>.log; table by experiments/latency/time_table.py.
set -euo pipefail
cd "$(dirname "$0")/../.."
NAME=$1; NS=$2; ROWS=$3; VARS=$4; REP=${5:-3}
D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; Q=$D/blocks/queries.fa; W=${W:-$HOME/data/latency/time}; O=$W/$NAME; mkdir -p $O $W/ref
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"; TS="taskset -c ${PIN:-0}"
args() { case $1 in nojumps) echo "--twolevel --no-jumps $D/c$2" ;; unique) echo "--twolevel $D/unique/c${2}u" ;;
         heavy09) echo "--twolevel --pshortcuts $D/psc/c$2.heavy09.psc $D/c$2" ;; move) echo "$D/c$2" ;; *) echo "unknown row $1" >&2; exit 1 ;; esac; }
echo "== $NAME start $(date '+%F %T'): NS '$NS', ROWS '$ROWS', VARIANTS '$VARS', REP $REP"
for n in $NS; do for c in $ROWS; do for a in bml fb; do
  F=""; [ $a = fb ] && F="--fb"; ref=$W/ref/q$n.$c.$a.tsv
  if [ ! -s $ref ]; then
    $CAP $TS bin/katka-base find -L 1000 --format mems --move --stats -j 1 $F $(args $c $n) $Q $ref > $W/ref/q$n.$c.$a.log 2>&1
    cmp -s <(cut -f1-5 $ref) <(cut -f1-5 $D/blocks/q${n}s.blocks.tsv) || { echo "== STOPPED: reference q$n $c $a differs from the paper's"; exit 1; }
  fi
done; done; done
for n in $NS; do for r in $(seq 1 $REP); do for c in $ROWS; do for v in $VARS; do
  lab=${v%%=*}; spec=${v#*=}; bin=${spec%%:*}; envs=""; [ "$spec" != "$bin" ] && envs=$(echo ${spec#*:} | tr ':' ' ')
  for a in bml fb; do
    F=""; [ $a = fb ] && F="--fb"; log=$O/q$n.$c.$lab.$a.$r.log
    env $envs $CAP $TS $bin find -L 1000 --format mems --move --stats -j 1 $F $(args $c $n) $Q $W/out.$NAME.tsv > $log 2>&1
    cmp -s $W/out.$NAME.tsv $W/ref/q$n.$c.$a.tsv || { echo "== STOPPED: q$n $c $lab $a round $r: output differs from bin/katka-base's"; exit 1; }
  done
done; done; done; echo "-- c$n done $(date +%T)"; done
rm -f $W/out.$NAME.tsv
echo "== $NAME done $(date '+%F %T')"
