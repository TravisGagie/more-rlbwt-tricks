#!/bin/bash
# Reportable Shortcuts with fixed-length almost-funnels on the text (2026-11-11): CHM13 as the reference,
# maximum tier tau = 0, so every shortcut crosses exactly one window, of ell = 25 (and ell = 100) characters, on the
# RLBWTs of the text and of its reverse (forward strands only), as in the paper's tau = 6 runs (experiments/rs_rerun.sh).
# From tool/:
#
#   experiments/rs_tau0.sh [SIZES] [ELLS]
#
# SIZES (default "16 32 64" individuals), ELLS (default "25,100").  For each size:
#   build: build_index --max-tier 0 --theta 0 --ell ELLS on the RLBWTs of the tau = 6 indexes (the RLBWT doesn't depend
#          on tau), forward and reversed, into ~/research/rlbwt-tricks/shortcuts/chr19_scaling/hprc_chr19/n<size>/
#          {fwd,rev}_t0, in an 11 GB cgroup without swap;
#   query: query_mems --algorithm bml,fb --min-length 1000 --repeat 3 with all ELLS, pinned to CPU $PIN (default 0), in
#          an 11 GB cgroup.  Each round runs plain search (no shortcuts) and then each set of shortcuts, so the plain
#          times are a same-day rerun; the medians exclude loading, and query_mems stops on any MEM mismatch with plain.
# Output: results/chr19/rs_tau0/n<size>.{csv,log}, build logs build<size>.{fwd,rev}.log, summary.txt.
set -euo pipefail
cd "$(dirname "$0")/.."
SIZES=${1:-"16 32 64"}; ELLS=${2:-"25,100"}
B=~/research/rlbwt-tricks/shortcuts/rs_build/reportable-shortcuts/build
H=~/research/rlbwt-tricks/shortcuts/chr19_scaling/hprc_chr19
Q=~/research/rlbwt-tricks/chr19/blocks/queries.txt
N=$(cat $H/ref_length.txt)
O=results/chr19/rs_tau0; mkdir -p $O
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"
NAMES=$(echo $ELLS | tr ',' '\n' | sed 's/$/_theta0/; s/^/ell/' | paste -sd,)
# not /tmp: it is a 7.5 GB tmpfs here, and the collection at 64 individuals is 7.9 GB
SCR=$(mktemp -d -p "${TMPDIR:-$HOME/research/rlbwt-tricks/tmp}" rs_tau0.XXXX)
for s in $SIZES; do
  X=$H/n$s
  C=$SCR/collection.txt; head -n $((1 + 2 * s)) $H/collection.txt > $C
  for side in fwd rev; do
    [ -s $X/${side}_t0/build.csv ] && continue
    FLAG=""; [ $side = rev ] && FLAG="--reversed"
    echo "[$(date '+%F %T')] $s individuals: build $side, tau = 0, ell = $ELLS" >&2
    rm -rf $X/${side}_t0
    /usr/bin/time -f "wall %e s, peak %M KB" $CAP $B/build_index --text $C --rlbwt $X/${side}_t6/rlbwt.bin --reference-length $N \
      --out $X/${side}_t0 --ell $ELLS --theta 0 --max-tier 0 $FLAG > $O/build$s.$side.log 2>&1 \
      || { echo "$s individuals: build $side failed"; tail -3 $O/build$s.$side.log; exit 1; }
  done
  rm -f $C
  echo "[$(date '+%F %T')] $s individuals: query" >&2
  /usr/bin/time -f "wall %e s, peak %M KB" $CAP taskset -c ${PIN:-0} $B/query_mems --index $X/fwd_t0 --reversed-index $X/rev_t0 \
    --shortcuts $NAMES --patterns $Q --algorithm bml,fb --min-length 1000 --repeat 3 --csv $O/n$s.csv > $O/n$s.log 2>&1
  grep -q MISMATCH $O/n$s.log && { echo "MISMATCH at $s"; exit 1; }
  awk -F, -v s=$s 'NR > 1 { k = $1 " " $2; m[k] += $6; p[k] += $8; f[k] += $9 }
    END { for (k in m) printf "%s individuals, %s: MEMs %d, plain %.2f s, shortcuts %.2f s\n", s, k, m[k], p[k] / 1e9, f[k] / 1e9 }' \
    $O/n$s.csv | sort | tee -a $O/summary.txt
done
rm -rf $SCR
