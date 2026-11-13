#!/bin/bash
# Builds the counting tools of 2026-11-11 ("how many backward steps could skipping save?"), from tool/:
#   bin/count-nonshrinking  experiments/count_nonshrinking.cpp, linked with Reportable Shortcuts' library (unchanged)
#   bin/katka-ns            katka with -DKATKA_NS (src/ns_count.hpp: parse-step interval sizes); bin/katka is unchanged
set -euo pipefail
cd "$(dirname "$0")/.."
R=~/research/rlbwt-tricks/shortcuts/rs_build/reportable-shortcuts
g++ -std=c++17 -O3 -march=native -DNDEBUG -I$R/include experiments/count_nonshrinking.cpp $R/build/libshortcuts.a $R/build/libsais64.a -o bin/count-nonshrinking
g++ -std=c++17 -O3 -march=native -DNDEBUG -Wall -Wno-unused-function -DKATKA_NS -Ithird_party/libsais/include -pthread \
  src/main.cpp build/libsais.o build/libsais64.o -o bin/katka-ns -lz -pthread
