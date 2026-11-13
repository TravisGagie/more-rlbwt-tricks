// Counting non-shrinking parse steps (2026-11-11, "how many backward steps could skipping save?", item 5).
// Compiled in only with -DKATKA_NS (experiments/build_count_nonshrinking.sh builds bin/katka-ns); bin/katka is unchanged.
// For every parse step [plo, phi] -> [o1, o2) of two-level search: |I| before and after, (nearly) non-shrinking
// steps for eps = 0, 1%, 5%, 10% (steps from an interval of size 1 counted apart and breaking runs), and the
// maximal runs of them within one search, with a histogram of their lengths (1, 2-3, ..., >= 1024).  Single-threaded.
#pragma once
#include <cstdint>
#include <cstdio>
namespace katka {
// Also (2026-11-12, item 4): what reading the parse Z directly would cost in this pass, per kind of search
// (dir 0: BML's backward searches, the right-to-left pass; 1: its forward searches, backward on the reverse
// complement), under two models:
//   single occurrence: once the interval is one row, read Z leftward from that occurrence instead of LF:
//     one seek per search that reaches one row (a stretch), and one symbol read per parse step taken from one row;
//   toehold: every parse step reads the symbol of Z before one kept occurrence (row t of the interval); if it is
//     the pattern's, t <- LF(t), else a departure: a seek to another occurrence (t <- the new interval's first row),
//     plus one seek per search entering the parse level.
struct rl_counts { uint64_t searches = 0, reads = 0, s1_reads = 0, s1_stretches = 0, toe_dep = 0, toe_dep_ok = 0; };
struct ns_counts {
    int dir = 0; rl_counts rl[2];
    static constexpr int NE = 4, NB = 12;
    const double eps[NE] = {0.0, 0.01, 0.05, 0.10};
    uint64_t steps = 0, failed = 0, size1 = 0, size1_failed = 0;
    uint64_t ns[NE] = {}, runs[NE] = {}, runlen[NE] = {}, hist[NE][NB] = {}, cur[NE] = {};
    static int bucket(uint64_t len) { int b = 0; while (b < NB - 1 && (uint64_t(2) << b) <= len) ++b; return b; }
    void step(uint64_t s0, uint64_t s1) {
        ++steps; if (!s1) ++failed;
        if (s0 == 1) { ++size1; if (!s1) ++size1_failed; }
        for (int e = 0; e < NE; ++e) {
            if (s0 > 1 && s1 && (double)s1 >= (1.0 - eps[e]) * (double)s0) { ++ns[e]; ++cur[e]; }
            else end(e);
        }
    }
    void end(int e) { if (cur[e]) { ++runs[e]; runlen[e] += cur[e]; ++hist[e][bucket(cur[e])]; cur[e] = 0; } }
    void end_search() { for (int e = 0; e < NE; ++e) end(e); }
    void print(FILE *f) const {
        for (int d = 0; d < 2; ++d) {
            const rl_counts &c = rl[d];
            fprintf(f, "parse reads, %s searches: %lu searches at the parse level, %lu parse steps; single occurrence: %lu stretches (seeks), "
                       "%lu symbols read; toehold: %lu seeks (%lu searches + %lu departures, %lu of them at a step that succeeds), %lu symbols read\n",
                    d ? "forward" : "backward", c.searches, c.reads, c.s1_stretches, c.s1_reads, c.searches + c.toe_dep, c.searches, c.toe_dep, c.toe_dep_ok, c.reads);
        }
        fprintf(f, "nonshrinking parse steps: %lu parse steps (%lu failed); from an interval of size 1: %lu (%lu failed)\n",
                steps, failed, size1, size1_failed);
        for (int e = 0; e < NE; ++e) {
            const uint64_t lb = steps - ns[e] + runs[e];
            fprintf(f, "  eps = %.0f%%: (nearly) non-shrinking %lu (%.2f%%), runs %lu, total run length %lu (mean %.2f); "
                       "lower bound steps - ns + runs = %lu, best-possible speedup in parse steps %.3f\n  run lengths:",
                    eps[e] * 100, ns[e], 100.0 * ns[e] / (steps ? steps : 1), runs[e], runlen[e], runs[e] ? (double)runlen[e] / runs[e] : 0.0,
                    lb, (double)steps / (lb ? lb : 1));
            for (int b = 0; b < NB; ++b) {
                if (b == NB - 1) fprintf(f, " >=%lu: %lu\n", uint64_t(1) << b, hist[e][b]);
                else if (b == 0) fprintf(f, " 1: %lu,", hist[e][b]);
                else fprintf(f, " %lu-%lu: %lu,", uint64_t(1) << b, (uint64_t(2) << b) - 1, hist[e][b]);
            }
        }
    }
};
inline ns_counts g_ns;
}  // namespace katka
