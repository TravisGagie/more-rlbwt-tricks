#!/bin/bash
# Two-level search's --locate (--csa --twolevel, c<N>.tloc) by the rules of fast_time.sh: huge pages, pinned, 11 GB
# cgroup, loading excluded, median of $REP (default 3), with and without alternating, bin/katka-ploc2, BML and FB, L = 1000,
# 16 and 32 individuals (there is no c129.tloc).  Times to $OUT/tloc/times.txt (N config alg round seconds peakKB).
set -euo pipefail
cd "$(dirname "$0")/../.."
D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; Q=$D/blocks/queries.fa; W=${W:-$HOME/diamond/ploc/fast}
OUT=${OUT:-results/chr19/parse_locate/fast}; REP=${REP:-3}; mkdir -p $W/ref $OUT/tloc
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"; TS="taskset -c ${PIN:-0}"; HP="GLIBC_TUNABLES=glibc.malloc.hugetlb=1"
for N in 33 65; do for r in $(seq 1 $REP); do
  cs="csa csa+loc"; [ $((r % 2)) = 0 ] && cs="csa+loc csa"
  for c in $cs; do for a in bml fb; do
    F=(); [ $a = fb ] && F=(--fb); X=(); [ $c = csa+loc ] && X=(--locate); lg=$OUT/tloc/$N.$c.$a.$r
    /usr/bin/time -f "%M" -o $lg.peak $CAP $TS env $HP bin/katka-ploc2 find -L 1000 --format mems --stats -j 1 "${F[@]}" --csa --twolevel "${X[@]}" $D/c$N $Q $W/o.tsv > $lg.log 2>&1
    ref=$W/ref/$N.$c.$a.tsv; [ -s $ref ] || cp $W/o.tsv $ref
    cmp -s $W/o.tsv $ref || { echo "== STOPPED: $N $c $a $r: output differs"; exit 1; }
    echo "$N $c $a $r $(grep -o '(total [0-9.]* s)' $lg.log | tr -d '()' | awk '{print $2}') $(cat $lg.peak)" | tee -a $OUT/tloc/times.txt >&2
  done; done
done; done
rm -f $W/o.tsv
