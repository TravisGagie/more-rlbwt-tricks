#!/bin/bash
# Stage 2, part C: verification of the text-free rows with pairs or shortcuts, as check.sh: every search repeated by
# two-level search with the index of the text (KATKA_TEXTLESS_CHECK=1), the output's MEMs equal plain search's, the
# occurrences equal its intervals' sizes.  CONFIGS: name:suffix:options:environment, e.g.
#   "unique:uv::" (▷, C2a), "unique-right:uv::KATKA_TL_RIGHT=1" (C2b), "unitig:wv::" (◇), "heavy08:v:--pshortcuts PSC:".
# PSC in options becomes $D/psc/c<N>.<name>.psc; @ in options or environment stands for a space.
# Forward-backward (ALGS=fb) sets KATKA_TL_FB=1.  One run at a time, pinned to CPU $PIN (default 0), 11 GB cgroup.
set -euo pipefail
cd "$(dirname "$0")/../.."
D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; Q=$D/blocks/queries.fa; REF=${REF:-$HOME/data/latency/final/ref}
DW=${DW:-$HOME/diamond}; W=${W:-$HOME/diamond/tl}; OUT=${OUT:-results/chr19/textless/c/check}; NS=${NS:-"33 65 129"}
mkdir -p $W $OUT
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"; TS="taskset -c ${PIN:-0}"
for n in $NS; do for c in $CONFIGS; do
  IFS=: read name suf opts envs <<< "$c"
  base=${name%%-*}; opts=${opts//PSC/$D/psc/c$n.$base.psc}; opts=${opts//@/ }; envs=${envs//@/ }; algs=${ALGS:-bml}
  for a in $algs; do
    F=""; E=""; [ $a = fb ] && F="--fb" && E="KATKA_TL_FB=1"; R=$REF/q$n.nojumps.move.$a.tsv
    $CAP $TS env KATKA_TEXTLESS_CHECK=1 $E $envs bin/katka find -L 1000 --format mems --stats -j 1 $F --move --twolevel $opts --textless $DW/c$n$suf $Q $W/out.tsv > $OUT/q$n.$name.$a.log 2>&1 \
      || { echo "== STOPPED: q$n $name $a exited with $? (see $OUT/q$n.$name.$a.log)"; exit 1; }
    cmp -s <(cut -f1-3 $W/out.tsv) <(cut -f1-3 $R) || { echo "== STOPPED: q$n $name $a: MEMs differ"; exit 1; }
    cmp -s <(awk -F'\t' '{print $5-$4}' $W/out.tsv) <(awk -F'\t' '{print $5-$4}' $R) || { echo "== STOPPED: q$n $name $a: occurrences differ"; exit 1; }
    echo "q$n $name $a: MEMs equal, occurrences equal; $(grep 'textless check' $OUT/q$n.$name.$a.log | sed 's/textless check: //')"
  done
done; done
rm -f $W/out.tsv
