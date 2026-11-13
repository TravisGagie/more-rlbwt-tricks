#!/bin/bash
# Successor rules for blocks on the chr19 indexes of More RLBWT Tricks' Table 1 (README.md here).  From tool/:
set -euo pipefail
g++ -std=c++17 -O3 -march=native -DNDEBUG -Isrc -Ithird_party/libsais/include experiments/successor_rules.cpp \
    build/libsais.o build/libsais64.o -o bin/successor-rules -lz -pthread
O=results/chr19/successor_rules
C="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"
for n in 33 65 129; do
  /usr/bin/time -f "wall %e s, peak %M KB" $C bin/successor-rules ~/research/rlbwt-tricks/chr19/c$n --check-order > $O/c$n.tsv 2> $O/c$n.log
done
