// The cost of a coroutine switch (src/coro.hpp; latency hiding on the move structure, part 2): K coroutines each yield
// N times, round-robin; prints the time and cycles (rdtsc, at the TSC's rate) per yield.  Also checks that every
// coroutine ran all its yields in turn.
//   g++ -std=c++17 -O3 -march=native -Isrc experiments/latency/co_bench.cpp -o bin/co_bench && bin/co_bench [N]
#include "coro.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <x86intrin.h>
int main(int argc, char **argv) {
    const long N = argc > 1 ? atol(argv[1]) : 20000000;
    for (int K : {1, 2, 8, 16, 32}) {
        katka::co_sched S;
        std::vector<long> done(K, 0);
        long order_err = 0, last = -1;
        auto t0 = std::chrono::steady_clock::now(); unsigned long long c0 = __rdtsc();
        S.run(K, [&](int id) {
            for (long i = 0; i < N; ++i) {
                if (K > 1 && last >= 0 && id != (last + 1) % K) ++order_err;
                last = id; ++done[id];
                katka::g_co->yield();
            }
        });
        unsigned long long c1 = __rdtsc(); auto t1 = std::chrono::steady_clock::now();
        long tot = 0; for (long d : done) tot += d;
        double s = std::chrono::duration<double>(t1 - t0).count();
        printf("K = %2d: %ld yields, %.2f ns and %.1f TSC cycles per yield; every coroutine ran %s; out of turn %ld\n",
               K, tot, 1e9 * s / tot, (double)(c1 - c0) / tot, tot == (long)K * N ? "all its yields" : "NOT all", order_err);
    }
}
