#!/bin/bash
# Latency hiding on the move structure, part 2, the checks of chopping (2.1) and interleaving (2.2), before any timing.
# Plain search on the move structure, find -L 1000 --format mems --stats -j 1 [--fb] --move, bin/katka-chop; every
# output must be byte-identical (MEMs and BWT intervals) to the unchopped search's:
#   A. 16 / 32 / 64 individuals, BML and FB: 64 pieces one by one (KATKA_CHOP=64), and interleaved with K = 8, 16, 32;
#      against the unchanged binary's outputs of the final timing session ($REF, bin/katka-base);
#   B. pieces far shorter than typical MEMs (KATKA_CHOP_LEN = 1000, 100, and 999, one below L), one by one and with
#      K = 8, on the first 2 Mbp of the first pattern (the work grows as M^2 / 2B), 16 / 32 / 64, BML and FB; against
#      bin/katka-base on the same queries;
#   C. cuts exactly at MEM starts, one character before and one after (KATKA_CHOP_AT: the starts of every 200th MEM
#      of the 16-individual BML output, the same offsets in every pattern), one by one and with K = 8, 16 individuals,
#      BML and FB;
#   D. brute force on a small synthetic collection (experiments/chr19_synthetic.py; 9 sequences of 30,000 characters,
#      both strands), L = 40: unchopped, pieces of 7 and 50 characters, and pieces of 7 with K = 4, BML and FB, against
#      p2_brute.py.
# One thread pinned to CPU $PIN (default 0), 11 GB cgroup.  Stops at the first difference.  Work in $W
# (default ~/data/latency/chop).
set -euo pipefail
cd "$(dirname "$0")/../.."
D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; Q=$D/blocks/queries.fa; W=${W:-$HOME/data/latency/chop}; REF=${REF:-$HOME/data/latency/final/ref}
mkdir -p $W; CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"; TS="taskset -c ${PIN:-0}"
B=bin/katka-chop; L=${L:-1000}
same() { cmp -s $1 $2 || { echo "== STOPPED: $1 differs from $2"; exit 1; }; }
run() {  # tag index queries reference, then environment words; extra options in $OPT
  local tag=$1 idx=$2 q=$3 ref=$4; shift 4
  env "$@" $CAP $TS $B find -L $L --format mems --stats -j 1 ${OPT:-} --move $idx $q $W/$tag.tsv > $W/$tag.log 2>&1
  same $W/$tag.tsv $ref
  echo "  $tag: identical; $(grep -o '(total [0-9.]* s)' $W/$tag.log); $(grep -o 'chopped search: .*' $W/$tag.log || echo unchopped)"
}
echo "== A. 64 pieces, one by one and interleaved"
for n in 33 65 129; do for a in bml fb; do
  OPT=""; [ $a = fb ] && OPT="--fb"
  run A.q$n.$a.chop $D/c$n $Q $REF/q$n.move.$a.tsv KATKA_CHOP=64
  for k in 8 16 32; do run A.q$n.$a.k$k $D/c$n $Q $REF/q$n.move.$a.tsv KATKA_CHOP=64 KATKA_CO_K=$k; done
done; done
echo "== B. short pieces, on the first 2 Mbp of the first pattern"
[ -s $W/sub.fa ] || { echo ">p0" > $W/sub.fa; awk 'NR==2' $Q | cut -c1-2000000 >> $W/sub.fa; }
for n in 33 65 129; do for a in bml fb; do
  OPT=""; [ $a = fb ] && OPT="--fb"
  $CAP $TS bin/katka-base find -L $L --format mems --stats -j 1 $OPT --move $D/c$n $W/sub.fa $W/B.q$n.$a.ref.tsv > $W/B.q$n.$a.ref.log 2>&1
  for len in 1000 100 999; do
    run B.q$n.$a.len$len $D/c$n $W/sub.fa $W/B.q$n.$a.ref.tsv KATKA_CHOP_LEN=$len
    run B.q$n.$a.len$len.k8 $D/c$n $W/sub.fa $W/B.q$n.$a.ref.tsv KATKA_CHOP_LEN=$len KATKA_CO_K=8
  done
done; done
echo "== C. cuts at MEM starts, one before, one after"
S0=$(awk 'NR % 200 == 1 {print $2}' $REF/q33.move.bml.tsv | sort -nu | tr '\n' ',' | sed 's/,$//')
echo "  $(echo $S0 | tr ',' '\n' | wc -l) MEM starts"
for a in bml fb; do
  OPT=""; [ $a = fb ] && OPT="--fb"
  for d in 0 -1 1; do
    C=$(echo $S0 | tr ',' '\n' | awk -v d=$d '{print $1 + d}' | tr '\n' ',' | sed 's/,$//')
    run C.$a.d$d $D/c33 $Q $REF/q33.move.$a.tsv KATKA_CHOP_AT=$C
    run C.$a.d$d.k8 $D/c33 $Q $REF/q33.move.$a.tsv KATKA_CHOP_AT=$C KATKA_CO_K=8
  done
done
echo "== D. brute force on a small synthetic collection"
Y=$W/synth; mkdir -p $Y
if [ ! -s $Y/c.mv ]; then
  python3 experiments/chr19_synthetic.py $Y/data --length 30000 --individuals 4 > $Y/gen.log
  mkdir -p $Y/fa; awk -v d=$Y/fa '{ f = sprintf("%s/s%03d.fa", d, NR); print ">s" NR > f; print > f; close(f) }' $Y/data/collection.txt
  ls $Y/fa/*.fa | sed 's/^/-\t/' > $Y/list.tsv
  { bin/katka-base prep $Y/list.tsv $Y/c && bin/katka-base bwt $Y/c && bin/katka-base index --preset chr19 $Y/c && bin/katka-base index --move $Y/c; } > $Y/build.log 2>&1
  tr -c 'ACGT\n' '\n' < $Y/data/queries.txt | awk 'length($0) > 0 { print ">p" NR - 1; print }' > $Y/queries.fa
fi
for a in bml fb; do
  OPT=""; [ $a = fb ] && OPT="--fb"
  L=40 $CAP $TS bin/katka-base find -L 40 --format mems --stats -j 1 $OPT --move $Y/c $Y/queries.fa $Y/$a.ref.tsv > $Y/$a.ref.log 2>&1
  python3 -I experiments/latency/p2_brute.py $Y/c.S $Y/queries.fa 40 $Y/$a.ref.tsv || { echo "== STOPPED: unchopped $a differs from brute force"; exit 1; }
  for v in "len7:KATKA_CHOP_LEN=7" "len50:KATKA_CHOP_LEN=50" "len7.k4:KATKA_CHOP_LEN=7:KATKA_CO_K=4"; do
    tag=${v%%:*}; E=$(echo ${v#*:} | tr ':' ' ')
    L=40 run D.$a.$tag $Y/c $Y/queries.fa $Y/$a.ref.tsv $E
    python3 -I experiments/latency/p2_brute.py $Y/c.S $Y/queries.fa 40 $W/D.$a.$tag.tsv || { echo "== STOPPED: $tag $a differs from brute force"; exit 1; }
  done
done
echo "== all checks passed $(date '+%F %T')"
