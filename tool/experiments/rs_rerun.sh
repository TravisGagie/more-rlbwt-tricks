#!/bin/bash
# Reportable Shortcuts' text shortcuts on chr19, rerun in More RLBWT Tricks' Table 1 setup (2026-11-08
# evening, item 3): its best configuration in the 2026-10-25 runs (tau = 6, ell = 25, theta = 0; the indexes of
# ~/research/rlbwt-tricks/shortcuts/chr19_scaling/hprc_chr19/n<size>/{fwd,rev}_t6), BML for MEMs of length >= 1000,
# on our four query patterns (blocks/queries.fa, one per line), 3 repeats, pinned to CPU $PIN (default 0), in an
# 11 GB cgroup without swap.  query_mems alternates plain search and search with shortcuts and keeps the median
# per pattern; it checks that both give the same MEMs.  Its code: reportable-shortcuts-code.tar.gz, built with
# CMake (Release) in ~/research/rlbwt-tricks/shortcuts/rs_build.  From tool/:  experiments/rs_rerun.sh "16 32 64"
set -euo pipefail
cd "$(dirname "$0")/.."
SIZES=${1:-"16 32 64"}
B=~/research/rlbwt-tricks/shortcuts/rs_build/reportable-shortcuts/build
H=~/research/rlbwt-tricks/shortcuts/chr19_scaling/hprc_chr19
Q=~/research/rlbwt-tricks/chr19/blocks/queries.txt
O=results/chr19/rs_rerun; mkdir -p $O
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"
for s in $SIZES; do
  echo "[$(date '+%F %T')] $s individuals" >&2
  /usr/bin/time -f "wall %e s, peak %M KB" $CAP taskset -c ${PIN:-0} $B/query_mems --index $H/n$s/fwd_t6 --reversed-index $H/n$s/rev_t6 \
    --shortcuts ell25_theta0 --patterns $Q --algorithm bml --min-length 1000 --repeat 3 --csv $O/n$s.csv > $O/n$s.log 2>&1
  cat $O/n$s.log
  grep -q MISMATCH $O/n$s.log && { echo "MISMATCH at $s"; exit 1; }
  awk -F, -v s=$s 'NR > 1 { m += $6; p += $8; f += $9 } END { printf "%s individuals: MEMs %d, plain %.2f s, shortcuts %.2f s\n", s, m, p / 1e9, f / 1e9 }' $O/n$s.csv | tee -a $O/summary.txt
done
