#!/bin/bash
# Two-level search without the index of the text, part B4: build PREFIX.tlx (bin/textless-build) for the paper's plain
# two-level indexes (copies in $DW, default ~/diamond), then BML with KATKA_TEXTLESS_CHECK=1: every search is also done
# by two-level search with the index of the text, and compared (j and the number of occurrences).  The output's MEMs
# (columns 1-3) must equal the final timing session's plain two-level output, and each MEM's number of occurrences
# (column 5 - column 4) its interval's size there.
# SUF=v: the prefix c<N>v (c<N>.lvl with the minimal-width c<N>tl.tl2; part C1).  One run at a time, pinned to CPU $PIN (default 0), 11 GB cgroup.
set -euo pipefail
cd "$(dirname "$0")/../.."
D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; Q=$D/blocks/queries.fa; REF=${REF:-$HOME/data/latency/final/ref}
DW=${DW:-$HOME/diamond}; W=${W:-$HOME/diamond/tl}; OUT=${OUT:-results/chr19/textless/b/check}; NS=${NS:-"33 65 129"}
mkdir -p $W $OUT
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"; TS="taskset -c ${PIN:-0}"
for n in $NS; do
  [ -n "${SUF:-}" ] || [ -s $DW/c$n.tlx ] || $CAP $TS /usr/bin/time -f "wall %e s, peak %M KB" bin/textless-build $DW/c$n $D/c$n.ptxt > $OUT/c$n.build.log 2>&1
  $CAP $TS env KATKA_TEXTLESS_CHECK=1 bin/katka find -L 1000 --format mems --stats -j 1 --move --twolevel --no-jumps --textless $DW/c$n${SUF:-} $Q $W/out.tsv > $OUT/q$n.check.log 2>&1
  R=$REF/q$n.nojumps.move.bml.tsv
  cmp -s <(cut -f1-3 $W/out.tsv) <(cut -f1-3 $R) || { echo "== STOPPED: q$n MEMs differ"; exit 1; }
  cmp -s <(awk -F'\t' '{print $5-$4}' $W/out.tsv) <(awk -F'\t' '{print $5-$4}' $R) || { echo "== STOPPED: q$n occurrences differ from the interval sizes"; exit 1; }
  echo "q$n: MEMs equal, occurrences equal the interval sizes; $(grep 'textless check' $OUT/q$n.check.log); psi $(cut -d' ' -f2 /proc/pressure/memory | head -1)"
done
rm -f $W/out.tsv
