// Runs per symbol in the two-level index's parse BWT (pbwt) and block BWT (bbwt), and the cache lines a symbol's range
// of runs occupies, for the latency study (results/chr19/latency/README.md, A.1 and A.2).  From tool/:
//   g++ -std=c++17 -O2 -march=native -Isrc experiments/latency/lat_counts.cpp -o bin/lat_counts -lz -pthread
//   bin/lat_counts PREFIX.lvl OUT [STEPS]
// Writes OUT.pbwt.tsv (and OUT.bbwt.tsv if the index has blocks): one line per symbol c that occurs,
//   c, runs, KS low-bit lines, KS high-bit lines, FP low-bit lines, FP high-bit lines,
// where the X lines are the 64-byte lines that the bits of c's runs occupy in that array (KS.low, KS.high, FP.low,
// FP.high).  Symbol c's runs are KS[a .. a + r), a = count_less(c n).
#include "twolevel.hpp"
#include <cstdio>
using namespace katka;

static uint64_t lines(uint64_t b0, uint64_t b1) { return b1 <= b0 ? 0 : (b1 - 1) / 512 - b0 / 512 + 1; }   // bits [b0, b1)

static void dump(const EFRank &E, uint64_t sig, const std::string &out) {
    FILE *o = fopen(out.c_str(), "w");
    if (!o) die("cannot write " + out);
    const EliasFano &K = E.KS, &F = E.FP;
    fprintf(o, "# n %llu sigma %llu runs %llu KS.l %u FP.l %u\n", (unsigned long long)E.n, (unsigned long long)sig,
            (unsigned long long)E.nruns, K.l, F.l);
    fprintf(o, "c\truns\tks_low_lines\tks_high_lines\tfp_low_lines\tfp_high_lines\n");
    uint64_t j = 0;
    while (j < E.nruns) {
        const uint64_t v = K.access(j), c = v / E.n;
        uint64_t a = j, ks0 = K.select_<true>(a), fp0 = F.select_<true>(a), ks1 = ks0, fp1 = fp0;
        while (j < E.nruns && K.access(j) / E.n == c) { ks1 = K.select_<true>(j); fp1 = F.select_<true>(j); ++j; }
        const uint64_t r = j - a;
        fprintf(o, "%llu\t%llu\t%llu\t%llu\t%llu\t%llu\n", (unsigned long long)c, (unsigned long long)r,
                (unsigned long long)lines(a * K.l, j * K.l), (unsigned long long)lines(ks0, ks1 + 1),
                (unsigned long long)lines(a * F.l, j * F.l), (unsigned long long)lines(fp0, fp1 + 1));
    }
    fclose(o);
}

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: lat_counts PREFIX.lvl OUT\n"); return 1; }
    two_level L;
    L.load(argv[1]);
    fprintf(stderr, "sigma %u, parse rows %llu, pbwt runs %llu, blocks %d, block symbols %llu, bbwt runs %llu\n", L.sigma,
            (unsigned long long)L.pbwt.n, (unsigned long long)L.pbwt.nruns, (int)L.has_blocks, (unsigned long long)L.nsym,
            (unsigned long long)(L.has_blocks ? L.bbwt.nruns : 0));
    dump(L.pbwt, L.sigma, std::string(argv[2]) + ".pbwt.tsv");
    if (L.has_blocks) dump(L.bbwt, L.nsym, std::string(argv[2]) + ".bbwt.tsv");
    return 0;
}
