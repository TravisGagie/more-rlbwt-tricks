#!/bin/bash
# A.1 and A.2 of the latency study: runs per symbol (bin/lat_counts) and parse steps per symbol (bin/katka-latcount,
# -DKATKA_LATCOUNT) for two-level plain (no jumps) and the unique-successor encoding, BML, L = 1000, move structure,
# 16 / 32 / 64 individuals.  Output in $W (default ~/data/latency/a); MEMs and intervals checked against the paper's.
set -euo pipefail
cd "$(dirname "$0")/../.."
D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; Q=$D/blocks/queries.fa; W=${W:-$HOME/data/latency/a}; mkdir -p $W
for n in 33 65 129; do
  bin/lat_counts $D/c$n.lvl $W/c$n 2>> $W/lat_counts.log
  bin/lat_counts $D/unique/c${n}u.lvl $W/c${n}u 2>> $W/lat_counts.log
  for c in nojumps unique; do
    case $c in nojumps) X="--twolevel --no-jumps $D/c$n"; o=$W/c$n.nojumps ;; unique) X="--twolevel $D/unique/c${n}u"; o=$W/c${n}u.unique ;; esac
    KATKA_LATCOUNT_OUT=$o bin/katka-latcount find -L 1000 --format mems --move --stats -j 1 $X $Q $W/out.tsv > $o.log 2>&1
    cmp -s <(cut -f1-5 $W/out.tsv) <(cut -f1-5 $D/blocks/q${n}s.blocks.tsv) || { echo "c$n $c: MEMs or intervals differ"; exit 1; }
    echo "c$n $c: MEMs and intervals equal the paper's; $(grep -o 'parse_steps[^,|]*' $o.log | head -1)"
  done
done
rm -f $W/out.tsv
