#!/bin/bash
# Forward-backward search with Cenzato et al.'s STPD-index (regindex, 8a432a4, RLZ + r-index, unmodified), chr19, in
# Table 1's setup (2026-11-10, item 2).  From tool/:
#
#   experiments/fb_stpd_index.sh [NS] [REPEATS]
#
# NS (default "33 65": 16 and 32 individuals; 64 doesn't fit, results/chr19/stpd_index_rerun/).  The indexes are
# those of the BML runs: ~/research/rlbwt-tricks/chr19/stpd<N>/st.RLZ-r-index.stpdi (experiments/stpd_index_rerun.sh).
# The driver is bin/stpd-index-mems --fb (experiments/stpd_index_mems.cpp):
#   forward:  the longest prefix of R[s..), their own left-to-right matching (O.LCP reads the text next to the
#             current occurrence; their binary search only at a mismatch, its candidate checked with O.LCS);
#   backward: the longest suffix of R[..e], as the longest prefix of its reverse complement over the same index of
#             both strands, with the same matching and checks (what BML's driver does).
# REPEATS (default 3) runs, pinned to CPU $PIN (default 0), in an 11 GB cgroup without swap, each limited to
# $LIMIT (default 3h).  The MEMs (name, start, end) must equal plain BML's (blocks/q<N>s.blocks.tsv).
set -euo pipefail
cd "$(dirname "$0")/.."
NS=${1:-"33 65"}; REP=${2:-3}; LIMIT=${LIMIT:-3h}
D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; Q=$D/blocks/queries.fa; O=results/chr19/fb; mkdir -p $O/logs
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"; TS="taskset -c ${PIN:-0}"
for n in $NS; do
  for r in $(seq 1 $REP); do
    log=$O/logs/q$n.stpd-index.fb.$r.log
    echo "[$(date '+%F %T')] c$n STPD-index FB repeat $r" >&2
    rc=0; /usr/bin/time -f "wall %e s, peak %M KB" $CAP $TS timeout $LIMIT bin/stpd-index-mems $D/stpd$n/st.RLZ-r-index.stpdi $Q 1000 --fb \
      > $O/q$n.stpd-index.fb.tsv 2> $log || rc=$?
    if [ $rc != 0 ]; then echo "c$n repeat $r: exit $rc (124 = over $LIMIT)" | tee -a $log >&2; break; fi
    cmp -s <(cut -f1-3 $O/q$n.stpd-index.fb.tsv) <(cut -f1-3 $D/blocks/q${n}s.blocks.tsv) || { echo "c$n: FB MEMs differ from BML's"; exit 1; }
    tail -1 $log >&2
  done
done
