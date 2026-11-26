// Builds the regular sample over the parse (src/parse_sample.hpp) from the two-level index's parse BWT and the parse:
//   bin/prs-build LVL PTXT OUT.prs [s = 64] [checks = 2000000]
// The suffix array of the parse by libsais; the sampled rows are those whose phrase index is a multiple of s, the last
// phrase's and $'s.  Then a check: from `checks` random rows (fixed seed), the walk gives SA's text position.
#include "parse_sample.hpp"
#include "parse_text.hpp"
#include "libsais.h"
using namespace katka;

struct ptxt_len {                // the phrase lengths of the parse, as textless_index[2]::len gives them
    const parse_text &P;
    inline u64 len(u64 y) const { return P.len_of(y); }
};

int main(int argc, char **argv) {
    if (argc < 4 || argc > 6) die("usage: prs-build LVL PTXT OUT.prs [s = 64] [checks = 2000000]");
    const u64 s = argc > 4 ? std::stoull(argv[4]) : 64, checks = argc > 5 ? std::stoull(argv[5]) : 2000000;
    const double t0 = now();
    two_level L; L.load(argv[1]);
    parse_text PT;
    if (!parse_text::current_format(argv[2])) die(std::string(argv[2]) + " is of the old format");
    PT.load(argv[2]);
    if (PT.m != L.m || PT.n != L.n || (int32_t)PT.K != L.K) die("the parse does not match the two-level index");
    const u64 n = L.n, m = L.m, N = m + 1;
    const EFRank &W = L.pbwt;
    if (W.n != N || !W.slot.empty()) die("unexpected parse BWT layout");
    std::vector<uint32_t> Z(N);
    for (u64 i = 0; i < m; ++i) Z[i] = (uint32_t)PT.zid(i);
    Z[m] = 0;
    PT.ids = PackedInts();
    std::vector<uint32_t> SA(N);
    if (libsais_int(reinterpret_cast<int32_t *>(Z.data()), reinterpret_cast<int32_t *>(SA.data()), (int32_t)N, (int32_t)L.sigma + 1, 0) != 0) die("libsais_int failed");
    std::vector<uint32_t>().swap(Z);
    fprintf(stderr, "suffix array of the parse: %.1f s\n", now() - t0);
    auto T = [&](u64 v) -> u64 { return v >= m ? n : PT.start(v); };
    parse_regular_sample P;
    P.s = s; P.N = N; P.n = n; P.m = m;
    {
        std::vector<u64> rw, tp;
        for (u64 x = 0; x < N; ++x) if (SA[x] % s == 0 || SA[x] + 1 >= m) { rw.push_back(x); tp.push_back(T(SA[x])); }
        P.mark.build(rw, N); P.pos.build(tp);
        printf("regular sample over the parse, s = %lu: %zu sampled rows of %lu; rows %.3f MB + text positions %.3f MB = %.3f MB\n",
               s, rw.size(), N, P.bytes_rows() / 1e6, P.bytes_pos() / 1e6, P.bytes() / 1e6);
    }
    P.save(argv[3]);
    // check from random rows (not $'s, row 0)
    const ptxt_len PL{PT};
    u64 bad = 0, steps = 0;
    uint64_t z = 0x2545f4914f6cdd1dull;
    const double t1 = now();
    for (u64 q = 0; q < checks; ++q) {
        z ^= z << 13; z ^= z >> 7; z ^= z << 17;
        const u64 r = z % N;
        const u64 p = P.locate(W, PL, r, (u64)L.K, steps);
        if (p != T(SA[r])) { if (!bad) printf("check: row %lu (phrase %u): %lu instead of %lu\n", r, SA[r], p, T(SA[r])); ++bad; }
    }
    printf("check from %lu random rows: %lu wrong; %.2f Psi steps per row, %.1f ns per row\n", checks, bad, (double)steps / std::max<u64>(1, checks),
           (now() - t1) * 1e9 / std::max<u64>(1, checks));
    fprintf(stderr, "done: %.1f s\n", now() - t0);
    return bad ? 1 : 0;
}
