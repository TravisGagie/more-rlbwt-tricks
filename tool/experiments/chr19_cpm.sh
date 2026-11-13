#!/bin/bash
# The chromosome 19 experiments of "More RLBWT Tricks" (arXiv:2609.40299v3): rebuilds every index and reruns every
# table with this tool.  See tool/CHR19.md, "The v3 experiments".
#
#   chr19_cpm.sh build  DATA WORK [SIZES]   the indexes, for each size (default "33 65 129" sequences = 16, 32, 64
#                                           individuals)
#   chr19_cpm.sh tables DATA WORK [SIZES]   the timings, MB loaded and checks of every configuration
#   chr19_cpm.sh all    DATA WORK [SIZES]   both
#
# DATA holds collection.txt and queries.txt as for experiments/chr19_paper.sh (CHM13 first).  WORK receives the
# indexes; OUT (default WORK/results) the logs and tables:
#   OUT/twolevel/table.md    every search configuration, BML and FB, median of REPEATS (default 3) interleaved rounds,
#                            MB loaded (experiments/twolevel_ac.sh and twolevel_ac_table.py):
#       plain backward search on the RLCSA, and on the move structure alone;
#       two-level search without block steps, with the unique-successor (▷) encoding, with heavy-path almost-funnels
#       (theta = 0.8, 0.9, 0.95) and with almost-funnels placed from CHM13's parse (g = 4, 16, 64), each with the
#       character steps at its ends on the RLCSA and on the move structure;
#       plain search on the move structure with almost-funnels on the text along CHM13 and its reverse complement
#       (windows of 25 and of 100 with tau = 0; windows of 25 with tau = 6);
#       our STPD of the parse (one occurrence per MEM, no intervals);
#   OUT/stpd_parse/          STPD over the text and over the parse at the smallest size (experiments/stpd_parse_timing.sh);
#   OUT/stpd_index/          the published STPD-index, if STPD is set (experiments/stpd_index_rerun.sh).
# Every search's MEMs and intervals are compared with plain backward search's (WORK/blocks/q<N>s.blocks.tsv, written by
# the build), and a difference stops the script.
#
# Environment:
#   BIGBWT   Big-BWT's bigbwt (needed for the real data; see chr19_paper.sh); PIN the CPU to pin searches to (default 0)
#   MEMMAX   the memory cap of each build step (default 11G); the searches run in 11 GB cgroups (systemd-run --user)
#   STPD     a built clone of github.com/regindex/STPD-index at 8a432a4 (see CHR19.md); without it, OUT/stpd_index is
#            skipped.  BIG: a directory with room for its suffix-array files (default WORK/stpd<N>/big)
#   REPEATS  rounds per configuration (default 3)
set -euo pipefail
CMD=$1; DATA=$(cd "$2" && pwd); mkdir -p "$3"; W=$(cd "$3" && pwd); SIZES=${4:-"33 65 129"}
HERE=$(cd "$(dirname "$0")/.." && pwd); K=$HERE/bin/katka; E=$HERE/experiments
OUT=${OUT:-$W/results}; mkdir -p "$OUT"; OUT=$(cd "$OUT" && pwd)
REPEATS=${REPEATS:-3}; export PIN=${PIN:-0}; export CHR19=$W SIZES
MEMMAX=${MEMMAX:-11G}
CAP=""; systemd-run --user --scope -q -p MemoryMax=$MEMMAX -p MemorySwapMax=0 true 2> /dev/null && CAP="systemd-run --user --scope -q -p MemoryMax=$MEMMAX -p MemorySwapMax=0"
SMALL=$(echo $SIZES | tr ' ' '\n' | sort -n | head -1)
stamp() { echo "[$(date '+%F %T')] $*"; }
step() { local log=$1; shift; echo "== $*" >> "$log"; /usr/bin/time -a -o "$log" -f "wall %e s, peak %M KB" $CAP "$@" >> "$log" 2>&1 || { echo "failed: $* (see $log)"; exit 1; }; }
PSC="heavy08:heavy:0.8 heavy09:heavy:0.9 heavy095:heavy:0.95 ref4:ref:4 ref16:ref:16 ref64:ref:64"
CONFIGS="rlcsa rlcsa.move nojumps.csa nojumps.move unique.csa unique.move heavy08.csa heavy08.move heavy09.csa heavy09.move
         heavy095.csa heavy095.move ref4.csa ref4.move ref16.csa ref16.move ref64.csa ref64.move tsc25t0 tsc100t0 tsc25t6 stpd"

build() {
  [ -x "$K" ] || { echo "build the tool first: make -C $HERE"; exit 1; }
  # the base indexes (prep, BWT, index --preset chr19: .rlbwt, .csa, .lvl) and, at the smallest size, the STPD,
  # parse-oracle, RLZ and locating structures
  "$E/chr19_paper.sh" build "$DATA" "$W" "$SIZES"
  mkdir -p "$W/blocks" "$W/unique" "$W/psc"
  [ -s "$W/blocks/queries.fa" ] || cp "$W/queries.fa" "$W/blocks/queries.fa"
  for n in $SIZES; do
    P=$W/c$n; L=$W/cpm_build$n.log
    [ -s $P.mv ] || { stamp "c$n: the move structure"; step $L $K index --move $P; }
    if [ ! -s $P.pstpd ]; then        # the STPD of the parse at every size (chr19_paper.sh builds it at the smallest)
      stamp "c$n: the STPD sample through the parse oracle, the STPD of the parse"
      step $L $K index --stpd --parse-oracle $P; step $L $K index --stpd-parse $P
    fi
    if [ ! -s $W/unique/c${n}u.lvl ]; then
      stamp "c$n: the unique-successor (▷) encoding"
      for e in csa info mv rlbwt tbl; do ln -sf ../c$n.$e $W/unique/c${n}u.$e; done
      step $L $K index --twolevel --continuation unique --blocks-from $P.lvl $W/unique/c${n}u
    fi
    for s in $PSC; do
      name=${s%%:*}; spec=${s#*:}
      [ -s $W/psc/c$n.$name.psc ] || { stamp "c$n: almost-funnels on the parse, $spec"; step $L $K index --pshortcuts $spec --pshortcuts-out $W/psc/c$n.$name.psc $P; }
    done
    # the reference output: plain BML on the RLCSA, L = 1000 (MEMs and intervals)
    [ -s $W/blocks/q${n}s.blocks.tsv ] || { stamp "c$n: plain BML, the reference MEMs"; step $L $K find -L 1000 --format mems --csa -j 1 $P $W/blocks/queries.fa $W/blocks/q${n}s.blocks.tsv; }
  done
  (cd "$HERE" && O=$OUT/tsc_text "$E/tsc_text_build.sh" "$SIZES")      # almost-funnels on the text: $W/tsc/
}

tables() {
  stamp "every configuration, BML and FB, $REPEATS rounds"
  (cd "$HERE" && O=$OUT/twolevel "$E/twolevel_ac.sh" "$SIZES" "$(echo $CONFIGS)" $REPEATS)
  python3 "$E/twolevel_ac_table.py" "$OUT/twolevel" $CONFIGS > "$OUT/twolevel/table.md"
  stamp "STPD over the text and over the parse, c$SMALL"
  (cd "$HERE" && O=$OUT/stpd_parse MODES="parse text" "$E/stpd_parse_timing.sh" "$SMALL" $REPEATS)
  if [ -n "${STPD:-}" ]; then
    [ -x "$HERE/bin/stpd-index-mems" ] || "$E/build_stpd_index_mems.sh" "$STPD" "$HERE/bin/stpd-index-mems"
    for n in $SIZES; do
      stamp "the published STPD-index, c$n"
      (cd "$HERE" && O=$OUT/stpd_index STPD=$STPD BIGBWT=${BIGBWT:-bigbwt} "$E/stpd_index_rerun.sh" $n "RLZ:r-index" $REPEATS)
    done
  else stamp "STPD not set: the published STPD-index is skipped"; fi
  stamp "done: $OUT/twolevel/table.md"
}

case $CMD in
  build) build ;;
  tables) tables ;;
  all) build; tables ;;
  *) sed -n '2,30p' "$0"; exit 1 ;;
esac
