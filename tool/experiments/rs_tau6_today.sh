#!/bin/bash
# Reportable Shortcuts' variable-length almost-funnels (ell = 25, tau = 6, theta = 0, the paper's configuration) on
# chr19, BML and forward-backward, timed as experiments/rs_tau0.sh times tau = 0 (2026-11-11, "last runs",
# item 1): query_mems --algorithm bml,fb --min-length 1000 --repeat 3 on the tau = 6 indexes
# (~/research/rlbwt-tricks/shortcuts/chr19_scaling/hprc_chr19/n<size>/{fwd,rev}_t6), each round plain search and then
# the shortcuts in one process, pinned to CPU $PIN (default 0), in an 11 GB cgroup; query_mems stops on any MEM
# mismatch with plain search.  From tool/:  experiments/rs_tau6_today.sh [SIZES]
# Output: results/chr19/rs_tau6_today/n<size>.{csv,log}, summary.txt.
set -euo pipefail
cd "$(dirname "$0")/.."
SIZES=${1:-"16 32 64"}
B=~/research/rlbwt-tricks/shortcuts/rs_build/reportable-shortcuts/build
H=~/research/rlbwt-tricks/shortcuts/chr19_scaling/hprc_chr19
Q=~/research/rlbwt-tricks/chr19/blocks/queries.txt
O=results/chr19/rs_tau6_today; mkdir -p $O
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"
for s in $SIZES; do
  X=$H/n$s
  echo "[$(date '+%F %T')] $s individuals: query" >&2
  /usr/bin/time -f "wall %e s, peak %M KB" $CAP taskset -c ${PIN:-0} $B/query_mems --index $X/fwd_t6 --reversed-index $X/rev_t6 \
    --shortcuts ell25_theta0 --patterns $Q --algorithm bml,fb --min-length 1000 --repeat 3 --csv $O/n$s.csv > $O/n$s.log 2>&1
  grep -q MISMATCH $O/n$s.log && { echo "MISMATCH at $s"; exit 1; }
  awk -F, -v s=$s 'NR > 1 { k = $1 " " $2; m[k] += $6; p[k] += $8; f[k] += $9 }
    END { for (k in m) printf "%s individuals, %s: MEMs %d, plain %.2f s, shortcuts %.2f s\n", s, k, m[k], p[k] / 1e9, f[k] / 1e9 }' \
    $O/n$s.csv | sort | tee -a $O/summary.txt
done
