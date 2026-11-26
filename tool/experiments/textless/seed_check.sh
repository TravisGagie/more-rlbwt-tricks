#!/bin/bash
# The hash seeds as parameters (2026-11-24): index --hash-seed (the dictionary's phrase hash) and index --kr-base (the
# shortcuts' Karp-Rabin base).  At 16 individuals (c33):
#   1. defaults: rebuild the two-level index, ▷'s blocks, the text-free index and heavy paths theta = 80% with the new
#      binary and the default seeds, and compare each file byte for byte with the one the paper's runs used;
#   2. another seed for each hash: build the same files, then verify plain two-level search, ▷ with the jump and heavy
#      paths theta = 80% without the index of the text, BML and FB, as check_rows.sh does (every search repeated by
#      two-level search with the index of the text, the MEMs and occurrences equal plain search's);
#   3. the new binary on the paper's files gives byte-identical output to the final session's (rows tl, tlur, tlh08);
# and at 16 / 32 / 64 (4): the dictionary lookups and fingerprint comparisons of the heavy-path rows (find --stats).
# bin/katka-seed is this commit's bin/katka.  One run at a time, pinned to CPU $PIN (default 0), 11 GB cgroup.  No timing.
set -euo pipefail   # (bin/textless-build must be built from this commit's headers: older ones cannot read a stored seed)
cd "$(dirname "$0")/../.."
D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; Q=$D/blocks/queries.fa; REF=${REF:-$HOME/data/latency/final/ref}
DW=${DW:-$HOME/diamond}; S=${S:-$HOME/diamond/seed}; OLD=${OLD:-$HOME/diamond/time3/out}; OUT=${OUT:-results/chr19/textless/seed}
K=bin/katka-seed; SEED=${SEED:-0x0123456789abcdef}; KR=${KR:-0x1d2c3b4a5968778}
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"; TS="taskset -c ${PIN:-0}"
mkdir -p $S $OUT
run() { echo "[$(date '+%F %T')] $*" >&2; $CAP $TS "$@"; }
ST=${STEPS:-1234}; has() { [[ $ST == *$1* ]]; }      # STEPS: which of 1-4 to run (default all)
same() { if cmp -s "$1" "$2"; then echo "identical: $1 = $2"; else echo "DIFFER: $1 $2"; fi; }

if has 1 || has 2; then
for p in c33d c33s; do for x in S bwt csa info tbl; do ln -sf $D/c33.$x $S/$p.$x; done; done
fi
if has 1; then
  run $K index --twolevel $S/c33d > $OUT/build.c33d.log 2>&1
  run $K index --twolevel --continuation unique --blocks-from $S/c33d.lvl $S/c33du > $OUT/build.c33du.log 2>&1
  run bin/textless-build $S/c33d $D/c33.ptxt $S/c33d.tl2 > $OUT/build.c33d.tl2.log 2>&1
  run bin/textless-build $S/c33du $D/c33.ptxt $S/c33du.tl2 > $OUT/build.c33du.tl2.log 2>&1
  run $K index --pshortcuts heavy:0.8 --pshortcuts-out $S/c33d.heavy08.psc $S/c33d > $OUT/build.c33d.psc.log 2>&1
  { same $S/c33d.lvl $D/c33.lvl; same $S/c33d.rlbwt $D/c33.rlbwt; same $S/c33du.lvl $D/unique/c33u.lvl
    same $S/c33d.tlx $DW/c33.tlx; same $S/c33d.tl2 $DW/c33v.tl2; same $S/c33du.tl2 $DW/c33uv.tl2
    same $S/c33d.heavy08.psc $D/psc/c33.heavy08.psc; } | tee $OUT/defaults.txt
  rm -f $S/c33d.tlx $S/c33du.tlx
fi
if has 2; then
  run $K index --twolevel --hash-seed $SEED $S/c33s > $OUT/build.c33s.log 2>&1
  run $K index --twolevel --continuation unique --blocks-from $S/c33s.lvl $S/c33su > $OUT/build.c33su.log 2>&1
  run bin/textless-build $S/c33s $D/c33.ptxt $S/c33s.tl2 > $OUT/build.c33s.tl2.log 2>&1
  run bin/textless-build $S/c33su $D/c33.ptxt $S/c33su.tl2 > $OUT/build.c33su.tl2.log 2>&1
  run $K index --pshortcuts heavy:0.8 --kr-base $KR --pshortcuts-out $S/c33s.heavy08.psc $S/c33s > $OUT/build.c33s.psc.log 2>&1
  rm -f $S/c33s.tlx $S/c33su.tlx
  for x in mv tbl; do ln -sf $D/c33.$x $S/c33s.$x; ln -sf $D/c33.$x $S/c33su.$x; done; ln -sf $S/c33s.rlbwt $S/c33su.rlbwt
  { cmp -s $S/c33s.lvl $D/c33.lvl && echo "c33s.lvl EQUALS c33.lvl (the seed is not stored?)" || echo "c33s.lvl differs from c33.lvl, as it should (the dictionary)"
    cmp -s $S/c33s.heavy08.psc $D/psc/c33.heavy08.psc && echo "c33s.heavy08.psc EQUALS the default" || echo "c33s.heavy08.psc differs from the default, as it should (the fingerprints)"
    same $S/c33s.tl2 $DW/c33v.tl2; same $S/c33su.tl2 $DW/c33uv.tl2; } | tee $OUT/seeded.txt
  for c in "plain:c33s:--no-jumps:" "unique-right:c33su::KATKA_TL_RIGHT=1" "heavy08:c33s:--pshortcuts@$S/c33s.heavy08.psc:"; do
    IFS=: read name pre opts envs <<< "$c"; opts=${opts//@/ }
    for a in bml fb; do
      F=""; E=""; [ $a = fb ] && F="--fb" && E="KATKA_TL_FB=1"; R=$REF/q33.nojumps.move.$a.tsv
      run env KATKA_TEXTLESS_CHECK=1 $E $envs $K find -L 1000 --format mems --stats -j 1 $F --move --twolevel $opts --textless $S/$pre $Q $S/out.tsv > $OUT/q33.$name.$a.log 2>&1
      m=$(cmp -s <(cut -f1-3 $S/out.tsv) <(cut -f1-3 $R) && echo equal || echo DIFFER)
      o=$(cmp -s <(awk -F'\t' '{print $5-$4}' $S/out.tsv) <(awk -F'\t' '{print $5-$4}' $R) && echo equal || echo DIFFER)
      echo "q33 $name $a: MEMs $m, occurrences $o; $(grep 'textless check' $OUT/q33.$name.$a.log | sed 's/textless check: //')" | tee -a $OUT/seeded.txt
    done
  done
  rm -f $S/out.tsv
fi
if has 3; then
  for row in tl tlur tlh08; do for a in bml fb; do
    F=""; [ $a = fb ] && F="--fb"; E="KATKA_TL_FB=1 KATKA_NO_FIRST_RUN=1"; [ $row = tlur ] && E="$E KATKA_TL_RIGHT=1"
    case $row in tl) o="--twolevel --no-jumps --textless $DW/c33tl" ;; tlur) o="--twolevel --textless $DW/c33utl" ;;
                 tlh08) o="--twolevel --pshortcuts $D/psc/c33.heavy08.psc --textless $DW/c33tl" ;; esac
    run env $E $K find -L 1000 --format mems --stats -j 1 $F $o $Q $S/out.tsv > $OUT/q33.default.$row.$a.log 2>&1
    same $S/out.tsv $OLD/q33.$row.$a.tsv | sed "s|$S/out.tsv|q33 $row $a, new binary|" | tee -a $OUT/defaults.txt
  done; done
  rm -f $S/out.tsv
fi
if has 4; then
  for n in 33 65 129; do for h in heavy08 heavy09 heavy095; do for a in bml fb; do
    F=""; [ $a = fb ] && F="--fb"
    run env KATKA_TL_FB=1 KATKA_NO_FIRST_RUN=1 $K find -L 1000 --format mems --stats -j 1 $F --twolevel --pshortcuts $D/psc/c$n.$h.psc --textless $DW/c${n}tl $Q $S/out.tsv > $OUT/q$n.count.$h.$a.log 2>&1
    echo "q$n $h $a: $(grep '^hashes:' $OUT/q$n.count.$h.$a.log)" | tee -a $OUT/counts.txt
  done; done; done
  rm -f $S/out.tsv
fi
