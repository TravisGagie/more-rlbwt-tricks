#!/bin/bash
# Timing of locating from the parse alone (2026-11-26): the added time of find --parse-locate (regular sample s = 64)
# on the text-free index c<N>tl, BML and forward-backward, L = 1000, against the same binary without it; at 16 and 32
# individuals also two-level search's --locate (--csa --twolevel, c<N>.tloc; the same binary, bin/katka-ploc) against the same without it.
# The paper's rules: one thread pinned to CPU $PIN (default 0), 11 GB cgroup without swap, no huge pages, loading
# excluded (find's "(total X s)"), median of $REP (default 3) rounds; within a round the order of the with/without runs
# alternates.  Every output is compared with the first one of its configuration.  Logs to $OUT/time/.
set -euo pipefail
cd "$(dirname "$0")/../.."
D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; Q=$D/blocks/queries.fa; DW=${DW:-$HOME/diamond}; W=${W:-$HOME/diamond/ploc}
OUT=${OUT:-results/chr19/parse_locate/regular}; REP=${REP:-3}; L=${L:-1000}; mkdir -p $W/ref $OUT/time
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"; TS="taskset -c ${PIN:-0}"
run1() {   # N config alg round
  local N=$1 c=$2 a=$3 r=$4 F=() lg=$OUT/time/$1.$2.$3.$L.$4.log
  [ $a = fb ] && F=(--fb)
  case $c in
    tl)      $CAP $TS env KATKA_TL_FB=1 KATKA_NO_FIRST_RUN=1 bin/katka-ploc find -L $L --format mems --stats -j 1 "${F[@]}" --twolevel --no-jumps --textless $DW/c${N}tl $Q $W/o.tsv > $lg 2>&1 ;;
    tl+ploc) $CAP $TS env KATKA_TL_FB=1 KATKA_NO_FIRST_RUN=1 bin/katka-ploc find -L $L --format mems --stats -j 1 "${F[@]}" --twolevel --no-jumps --textless --parse-locate $DW/c${N}tl.prs $DW/c${N}tl $Q $W/o.tsv > $lg 2>&1 ;;
    csa)     $CAP $TS bin/katka-ploc find -L $L --format mems --stats -j 1 "${F[@]}" --csa --twolevel $D/c$N $Q $W/o.tsv > $lg 2>&1 ;;
    csa+loc) $CAP $TS bin/katka-ploc find -L $L --format mems --stats -j 1 "${F[@]}" --csa --twolevel --locate $D/c$N $Q $W/o.tsv > $lg 2>&1 ;;
  esac
  local ref=$W/ref/$N.$c.$a.$L.tsv
  [ -s $ref ] || cp $W/o.tsv $ref
  cmp -s $W/o.tsv $ref || { echo "== STOPPED: $N $c $a round $r: output differs from its first run"; exit 1; }
  echo "$N $c $a $r $(grep -o '(total [0-9.]* s)' $lg | tr -d '()' | awk '{print $2}')" | tee -a $OUT/time/times.txt >&2
}
for N in ${NS:-33 65 129}; do
  pairs="tl tl+ploc"; [ $N != 129 ] && pairs="$pairs csa csa+loc"
  for r in $(seq 1 $REP); do
    set -- $pairs; i=0
    while [ $# -gt 0 ]; do
      x=$1 y=$2; shift 2; i=$((i + 1))
      for a in bml fb; do
        if [ $(( (i + r) % 2 )) = 0 ]; then run1 $N $x $a $r; run1 $N $y $a $r; else run1 $N $y $a $r; run1 $N $x $a $r; fi
      done
    done
  done
done
rm -f $W/o.tsv
echo done >&2
