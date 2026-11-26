#!/bin/bash
# Builds bin/katka-surv: katka with -DKATKA_SURV (src/surv_count.hpp: the parse interval when each reported MEM's own
# two-level search leaves the parse).  bin/katka is unchanged.
set -euo pipefail
cd "$(dirname "$0")/../.."
make build/libsais.o build/libsais64.o
g++ -std=c++17 -O3 -march=native -DNDEBUG -Wall -Wno-unused-function -DKATKA_SURV -Ithird_party/libsais/include -pthread \
    src/main.cpp build/libsais.o build/libsais64.o -o bin/katka-surv -lz -pthread
# bin/parse-samples: the parse's sizes and samples, phi^-1 and Psi over the parse, the sample gaps
g++ -std=c++17 -O3 -march=native -DNDEBUG -Wall -Wno-unused-function -Ithird_party/libsais/include -Isrc -pthread \
    experiments/parse_locate/parse_samples.cpp build/libsais.o build/libsais64.o -o bin/parse-samples -lz -pthread
# bin/prs-build: the regular sample over the parse (src/parse_sample.hpp)
g++ -std=c++17 -O3 -march=native -DNDEBUG -Wall -Wno-unused-function -Ithird_party/libsais/include -Isrc -pthread \
    experiments/parse_locate/prs_build.cpp build/libsais.o build/libsais64.o -o bin/prs-build -lz -pthread
# bin/walk-bench: the Psi walks of locating from the parse alone, timed apart (one row at a time, staged, a run at a time)
g++ -std=c++17 -O3 -march=native -DNDEBUG -Wall -Wno-unused-function -Ithird_party/libsais/include -Isrc -pthread \
    experiments/parse_locate/walk_bench.cpp -o bin/walk-bench -lz -pthread
