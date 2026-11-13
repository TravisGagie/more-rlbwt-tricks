#!/bin/bash
# Almost-funnels on the text over one index of both strands (2026-11-12, "almost-funnels on the text, both
# strands"): katka index --text-shortcuts ELL,TAU on the move structure PREFIX.mv, along CHM13 (the first sequence of
# PREFIX.S) and its reverse complement.  From tool/:   experiments/tsc_text_build.sh [NS] [SPECS]
# NS (default "33 65 129" sequences = 16, 32, 64 individuals), SPECS (default "25,0 100,0 25,6").  11 GB cgroups.
# Output ~/research/rlbwt-tricks/chr19/tsc/c<N>.ell<ELL>t<TAU>.tsc, logs $O/build_logs/.
set -euo pipefail
cd "$(dirname "$0")/.."
D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}
NS=${1:-"33 65 129"}; SPECS=${2:-"25,0 100,0 25,6"}
O=${O:-results/chr19/tsc_text}; mkdir -p $O/build_logs $D/tsc
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"
for n in $NS; do for sp in $SPECS; do
  e=${sp%,*}; t=${sp#*,}; out=$D/tsc/c$n.ell${e}t$t.tsc
  [ -s $out ] && continue
  echo "[$(date '+%F %T')] c$n ell $e tau $t" >&2
  /usr/bin/time -f "wall %e s, peak %M KB" $CAP bin/katka index --text-shortcuts $sp --text-shortcuts-out $out $D/c$n \
    > $O/build_logs/c$n.ell${e}t$t.log 2>&1 || { echo "build c$n $sp failed"; tail -3 $O/build_logs/c$n.ell${e}t$t.log; exit 1; }
  tail -2 $O/build_logs/c$n.ell${e}t$t.log >&2
done; done
