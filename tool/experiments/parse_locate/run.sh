#!/bin/bash
# Survivors of the parse (2026-11-25): for every reported MEM of two-level search with blocks (--csa --twolevel), BML,
# the parse interval when the MEM's own two-level search leaves the parse (bin/katka-surv, src/surv_count.hpp).
# 16 / 32 / 64 individuals at L = 1000 and 100; at 16 also ▷ (the unique-continuation index) and heavy paths
# theta = 80% (shortcuts), to compare their rows MEM by MEM.  One run at a time, pinned to CPU $PIN (default 0), in an
# 11 GB cgroup.  No timing.  Per-MEM tables (gzipped) and logs go to $OUT.
set -euo pipefail
cd "$(dirname "$0")/../.."
D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; Q=$D/blocks/queries.fa; W=${W:-$HOME/diamond/surv}; OUT=${OUT:-results/chr19/parse_locate}
K=bin/katka-surv; CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"; TS="taskset -c ${PIN:-0}"
mkdir -p $W $OUT
one() {   # name N L prefix [options]
    local name=$1 N=$2 L=$3 pre=$4; shift 4
    echo "[$(date '+%F %T')] $name $N L=$L" >&2
    KATKA_SURV_OUT=$W/$name.$N.$L.tsv $CAP $TS $K find -L $L --format mems --csa --twolevel --stats -j 1 "$@" $pre $Q $W/mems.tsv > $OUT/$name.$N.$L.log 2>&1
    local m; m=$(cmp -s <(cut -f1-5 $W/mems.tsv) <(cut -f1-5 $D/plain$L.$N.tsv) && echo equal || echo DIFFER)
    paste <(awk -F'\t' '{print $5-$4}' $W/mems.tsv) $W/$name.$N.$L.tsv | awk -F'\t' -v m=$m -v t="$name $N L=$L" \
        '{o+=$1; if ($1 != $5) bad++; if ($4 == 2 && $6 < $5) lt++} END {printf "%s: MEMs %s, %d MEMs, occurrences %d, occ differs %d, survivors < occ %d\n", t, m, NR, o, bad, lt}' | tee -a $OUT/checks.txt
    gzip -c $W/$name.$N.$L.tsv > $OUT/$name.$N.$L.tsv.gz
}
for N in 33 65 129; do for L in 1000 100; do one blocks $N $L $D/c$N; done; done
for L in 1000 100; do
    one unique 33 $L $D/unique/c33u
    one heavy08 33 $L $D/c33 --pshortcuts $D/psc/c33.heavy08.psc
done
rm -f $W/mems.tsv
echo done >&2
