#!/bin/bash
# Does the forward search that finds a MEM already leave the parse at the MEM's parse interval?  For BML and
# forward-backward (--fb), two-level search with blocks, 16 / 32 / 64 individuals, L = 1000 and 100: bin/katka-surv
# compares, for every reported MEM, the interval [plo, phi] where the forward search left the parse with the one where
# the MEM's own two-level search leaves it ("surv forward check" in the logs).  One run at a time, pinned, 11 GB cgroup.
set -euo pipefail
cd "$(dirname "$0")/../.."
D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; Q=$D/blocks/queries.fa; W=${W:-$HOME/diamond/surv}; OUT=${OUT:-results/chr19/parse_locate}
K=bin/katka-surv; CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"; TS="taskset -c ${PIN:-0}"
mkdir -p $W $OUT; : > $OUT/forward_check.txt
for N in 33 65 129; do for L in 1000 100; do for alg in bml fb; do
    opt=(); [ $alg = fb ] && opt=(--fb)
    echo "[$(date '+%F %T')] $alg $N L=$L" >&2
    KATKA_SURV_OUT=$W/fw.tsv $CAP $TS $K find -L $L --format mems --csa --twolevel -j 1 "${opt[@]}" $D/c$N $Q $W/mems.tsv > $OUT/forward.$alg.$N.$L.log 2>&1
    m=$(cmp -s <(cut -f1-5 $W/mems.tsv) <(cut -f1-5 $D/plain$L.$N.tsv) && echo equal || echo DIFFER)
    echo "$alg $N L=$L: MEMs $m; $(grep 'surv forward check' $OUT/forward.$alg.$N.$L.log)" | tee -a $OUT/forward_check.txt
done; done; done
rm -f $W/mems.tsv $W/fw.tsv
echo done >&2
