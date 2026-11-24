// Stage 2, part C2: on a two-level index with pairs, how many phrases that start pairs have more than one pair symbol
// (pair lengths j and sides), and how the extra symbols' lengths relate to the first.
//   bin/pair-symbols PREFIX.lvl
#include "twolevel.hpp"
using namespace katka;
int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: pair-symbols PREFIX.lvl\n"); return 1; }
    setenv("KATKA_NO_FIRST_RUN", "1", 1);
    two_level L; L.load(argv[1]);
    u64 starts = 0, multi = 0, extra = 0, maxs = 0, sided_multi = 0, ex_d = 0;
    for (u64 d = 1; d <= L.sigma; ++d) {
        u64 a, b; L.dstart.access2(d, a, b);
        if (b == a) continue;
        ++starts;
        if (b - a > 1) {
            ++multi; extra += b - a - 1; maxs = std::max(maxs, b - a);
            bool sides = false; for (u64 t = a; t < b; ++t) if (!L.sym_plus(t)) sides = true;
            if (sides) ++sided_multi;
            if (multi <= 5) { fprintf(stderr, "d %lu: symbols", d); for (u64 t = a; t < b; ++t) fprintf(stderr, " (j %lu, %c)", (u64)L.jv.get(t), L.sym_plus(t) ? '+' : '-'); fprintf(stderr, "; succ %u\n", L.succ((uint32_t)d)); ++ex_d; }
        }
    }
    printf("sided\t%u\nphrases_starting_pairs\t%lu\nwith_more_than_one_symbol\t%lu\nextra_symbols\t%lu\nmost_symbols\t%lu\nmulti_with_minus_side\t%lu\n", L.sided, starts, multi, extra, maxs, sided_multi);
    return 0;
}
