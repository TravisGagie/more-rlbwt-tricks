#!/bin/bash
# Profile of our STPD of the parse (2026-11-12, "faster STPD of the parse", item 1).  From tool/:
#   experiments/stpd_parse_profile.sh [N]     (N = 33: 16 individuals)
# BML, L = 1000, one occurrence per MEM (`find --csa --stpd-parse --no-intervals`), as in stpd_parse_fb.sh, pinned to
# CPU $PIN (default 0), in 11 GB cgroups:
#   bin/katka three times (unprofiled times);
#   bin/katka-prof (src/prof.hpp: -DKATKA_PROF, TSC timers around the parts) three times without writing queries,
#     then once writing every LCE query to ~/research/rlbwt-tricks/chr19/prof/c<N>.{queries,patterns};
# every run's MEMs checked against the paper's BML output.  Then experiments/stpd_parse_profile.py summarizes.
# Output in results/chr19/stpd_parse_profile/.
set -euo pipefail
cd "$(dirname "$0")/.."
N=${1:-33}
D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; Q=$D/blocks/queries.fa; O=results/chr19/stpd_parse_profile; DP=$D/prof; mkdir -p $O/logs $DP
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"; TS="taskset -c ${PIN:-0}"
g++ -std=c++17 -O3 -march=native -DNDEBUG -Wall -Wno-unused-function -DKATKA_PROF -Ithird_party/libsais/include -pthread \
  src/main.cpp build/libsais.o build/libsais64.o -o bin/katka-prof -lz -pthread
SCR=$(mktemp -d -p "${TMPDIR:-$HOME/research/rlbwt-tricks/tmp}" stpd_parse_profile.XXXX)
run() {   # binary, log, [env]
  echo "[$(date '+%F %T')] c$N $2" >&2
  env ${3:-} /usr/bin/time -f "wall %e s, peak %M KB" $CAP $TS $1 find -L 1000 --format mems --csa --stpd-parse --stats -j 1 --no-intervals \
    $D/c$N $Q $SCR/out.tsv > $O/logs/q$N.$2.log 2>&1
  cmp -s <(cut -f1-3 $SCR/out.tsv) <(cut -f1-3 $D/blocks/q${N}s.blocks.tsv) || { echo "c$N $2: MEMs differ from BML's"; exit 1; }
}
for r in 1 2 3; do run bin/katka plain.$r; done
for r in 1 2 3; do run bin/katka-prof prof.$r; done
run bin/katka-prof prof.queries KATKA_PROF_OUT=$DP/c$N
rm -rf $SCR
python3 experiments/stpd_parse_profile.py $N $DP/c$N $O > $O/summary.c$N.md
echo "[$(date '+%F %T')] c$N: done" >&2
