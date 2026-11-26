#!/bin/bash
# Locating from the parse alone with a regular sample (2026-11-26): builds PREFIX.prs (bin/prs-build, s = 64) for the
# text-free index c<N>tl (stage 2, part C: c<N>tl.lvl + c<N>tl.tl2 in $DW) and checks find --parse-locate, BML and
# forward-backward, 16 / 32 / 64 individuals, L = 1000 ($LS; at L = 100 the minimal-width index's own search stops:
# "alpha has no identifier in an exact search"):
#   - the MEMs and occurrences (columns 1-3, 5) equal the reference's (plain search's $D/plain$L.$N.tsv, its interval size);
#   - every position holds rc(MEM) in the text, by the parse oracle (KATKA_PLOC_CHECK = c<N>.ptxt), the positions are
#     distinct and as many as the occurrences;
#   - at 16 and 32, the positions equal two-level search's --locate-out ($D/locpos/tlpos$L.$N.txt), MEM by MEM.
# One run at a time, pinned to CPU $PIN (default 0), 11 GB cgroup.  Logs to $OUT.
set -euo pipefail
cd "$(dirname "$0")/../.."
D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; Q=$D/blocks/queries.fa; DW=${DW:-$HOME/diamond}; W=${W:-$HOME/diamond/ploc}
OUT=${OUT:-results/chr19/parse_locate/regular}; mkdir -p $W $OUT
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"; TS="taskset -c ${PIN:-0}"
for N in ${NS:-33 65 129}; do
  if [ ! -s $DW/c${N}tl.prs ]; then
    echo "[$(date '+%F %T')] build $N" >&2
    $CAP $TS /usr/bin/time -f "wall %e s, peak %M KB" bin/prs-build $DW/c${N}tl.lvl $D/c$N.ptxt $DW/c${N}tl.prs > $OUT/build.$N.txt 2> $OUT/build.$N.err
  fi
  for L in ${LS:-1000}; do for alg in bml fb; do
    F=(); [ $alg = fb ] && F=(--fb)
    echo "[$(date '+%F %T')] check $alg $N L=$L" >&2
    $CAP $TS env KATKA_TL_FB=1 KATKA_NO_FIRST_RUN=1 KATKA_PLOC_CHECK=$D/c$N.ptxt /usr/bin/time -f "wall %e s, peak %M KB" \
      bin/katka-ploc find -L $L --format mems --stats -j 1 "${F[@]}" --twolevel --no-jumps --textless --parse-locate $DW/c${N}tl.prs \
      --locate-out $W/pos.$alg.$N.$L.txt $DW/c${N}tl $Q $W/out.tsv > $OUT/check.$alg.$N.$L.log 2>&1
    m=$(cmp -s <(cut -f1-3 $W/out.tsv) <(cut -f1-3 $D/plain$L.$N.tsv) && cmp -s <(cut -f5 $W/out.tsv) <(awk -F'\t' '{print $5-$4}' $D/plain$L.$N.tsv) && echo equal || echo DIFFER)
    p="no two-level positions to compare"
    if [ -s $D/locpos/tlpos$L.$N.txt ]; then p=$(cmp -s <(sort $W/pos.$alg.$N.$L.txt) <(sort $D/locpos/tlpos$L.$N.txt) && echo "positions equal two-level --locate's" || echo "positions DIFFER from two-level --locate's"); fi
    echo "$alg $N L=$L: MEMs and occurrences $m; $p; $(grep -o 'parse locate: .*' $OUT/check.$alg.$N.$L.log)" | tee -a $OUT/checks.txt
  done; done
done
rm -f $W/out.tsv
echo done >&2
