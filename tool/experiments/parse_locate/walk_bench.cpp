// The Psi walks of locating from the parse alone (src/parse_sample.hpp), timed apart from the search:
//   bin/walk-bench LVL TL2 PRS [rows = 2000000] [ROWS.txt]
// From `rows` random rows (fixed seed), or the rows listed in ROWS.txt (one per line, as find --parse-locate writes them
// with $KATKA_PLOC_ROWS), the walks one at a time (locate, locate1) and `lanes` at a time (locate_many); every variant's
// positions must equal locate's.  Prints ns per row and per Psi step.
#include "parse_sample.hpp"
#include "textless.hpp"
using namespace katka;

int main(int argc, char **argv) {
    if (argc < 4 || argc > 6) die("usage: walk-bench LVL TL2 PRS [rows = 2000000] [ROWS.txt]");
    two_level L; L.load(argv[1]);
    textless_index2 T; T.load(argv[2]);
    parse_regular_sample P; P.load(argv[3]);
    if (P.N != L.pbwt.n || P.m != L.m || P.n != L.n) die("the sample does not match the index");
    std::vector<u64> rows;
    if (argc > 5) { FILE *f = open_or_die(argv[5], "r"); unsigned long x; while (fscanf(f, "%lu", &x) == 1) rows.push_back(x); fclose(f); }
    else {
        const u64 cnt = argc > 4 ? std::stoull(argv[4]) : 2000000;
        uint64_t z = 0x2545f4914f6cdd1dull;
        for (u64 q = 0; q < cnt; ++q) { z ^= z << 13; z ^= z >> 7; z ^= z << 17; rows.push_back(1 + z % (P.N - 1)); }
    }
    const u64 K = (u64)L.K, n = rows.size();
    printf("%lu rows, s = %lu, %lu runs\n", n, P.s, L.pbwt.nruns);
    std::vector<u64> ref(n), out(n);
    u64 steps = 0;
    double t = now();
    for (u64 q = 0; q < n; ++q) ref[q] = P.locate(L.pbwt, T, rows[q], K, steps);
    t = now() - t;
    printf("locate:        %7.1f ns per row, %6.1f ns per step (%.2f steps per row)\n", t * 1e9 / n, t * 1e9 / steps, (double)steps / n);
    const u64 S = steps;
    for (int rep = 0; rep < 2; ++rep) {
        steps = 0; t = now();
        for (u64 q = 0; q < n; ++q) out[q] = P.locate1(L.pbwt, T, rows[q], K, steps);
        t = now() - t;
        if (out != ref || steps != S) die("locate1 differs");
        printf("locate1:       %7.1f ns per row, %6.1f ns per step\n", t * 1e9 / n, t * 1e9 / steps);
        for (unsigned lanes : {1u, 2u, 4u, 8u, 12u, 16u, 24u, 32u, 64u}) {
            std::fill(out.begin(), out.end(), 0); steps = 0; t = now();
            P.locate_many(L.pbwt, T, rows.data(), n, K, out.data(), steps, lanes);
            t = now() - t;
            if (out != ref || steps != S) die("locate_many differs at " + std::to_string(lanes) + " lanes");
            printf("many, %2u lanes: %7.1f ns per row, %6.1f ns per step\n", lanes, t * 1e9 / n, t * 1e9 / steps);
        }
        for (unsigned lanes : {1u, 2u, 4u, 8u, 12u, 16u, 24u, 32u, 48u, 64u}) {
            std::fill(out.begin(), out.end(), 0); steps = 0; t = now();
            P.locate_staged(L.pbwt, T, rows.data(), n, K, out.data(), steps, lanes);
            t = now() - t;
            if (out != ref || steps != S) die("locate_staged differs at " + std::to_string(lanes) + " lanes");
            printf("staged, %2u lanes: %7.1f ns per row, %6.1f ns per step\n", lanes, t * 1e9 / n, t * 1e9 / steps);
        }
    }
    if (argc > 5) {                                 // one MEM's rows per line: the walks a run at a time
        std::vector<std::pair<u64, u64>> groups;   // (start, count) in rows
        FILE *f = open_or_die(argv[5], "r"); char *line = nullptr; size_t cap = 0; u64 at = 0;
        rows.clear();
        while (getline(&line, &cap, f) > 0) { const u64 st0 = rows.size(); char *p = line, *e; for (;;) { unsigned long x = strtoul(p, &e, 10); if (e == p) break; rows.push_back(x); p = e; } groups.push_back({st0, rows.size() - st0}); }
        fclose(f); free(line); (void)at;
        const u64 m = rows.size();
        std::vector<u64> r1(m), o1(m);
        u64 s1 = 0; double t1 = now();
        for (u64 q = 0; q < m; ++q) r1[q] = P.locate(L.pbwt, T, rows[q], K, s1);
        t1 = now() - t1;
        printf("MEM rows: %zu MEMs, %lu rows; one at a time %.3f s, %lu steps\n", groups.size(), m, t1, s1);
        for (unsigned lanes : {8u, 16u}) {
            u64 s3 = 0; double t3 = now();
            P.locate_staged(L.pbwt, T, rows.data(), m, K, o1.data(), s3, lanes);
            t3 = now() - t3;
            if (o1 != r1) die("staged differs");
            printf("  staged, %u lanes: %.3f s\n", lanes, t3);
        }
        for (int rep = 0; rep < 2; ++rep) {
            std::fill(o1.begin(), o1.end(), 0);
            u64 s2 = 0; double t2 = now();
            for (auto [a, c] : groups) P.locate_runs(L.pbwt, T, rows.data() + a, c, K, o1.data() + a, s2);
            t2 = now() - t2;
            if (o1 != r1) die("locate_runs differs");
            printf("  a run at a time: %.3f s, %lu piece steps\n", t2, s2);
        }
        for (unsigned lanes : {4u, 8u, 12u, 16u, 24u, 32u}) {
            std::fill(o1.begin(), o1.end(), 0);
            u64 s2 = 0; double t2 = now();
            P.locate_staged_runs(L.pbwt, T, rows.data(), m, groups.data(), groups.size(), K, o1.data(), s2, lanes);
            t2 = now() - t2;
            if (o1 != r1) die("locate_staged_runs differs");
            printf("  a run at a time, staged, %2u lanes: %.3f s, %lu piece steps\n", lanes, t2, s2);
        }
    }
    return 0;
}
