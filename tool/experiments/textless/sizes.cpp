// Two-level search without the index of the text, part A: sizes from existing files, no search code.
//   bin/textless-sizes PREFIX.lvl PREFIX.ptxt RANGES.tsv > out.tsv
// From the two-level index (any continuation rule): the bytes of each component as loaded (KATKA_NO_FIRST_RUN: the
// first-run tables are built at load and are not part of the file).  From the parse oracle's dictionary (the
// distinct phrases in id order): the stored phrases in three versions at 2 bits per base, with offsets and the
// phrases with other characters (X, $) as the parse oracle stores them; one copy that serves both ends.  From
// experiments/diamond/syncmer_ranges.cpp's output (RANGES.tsv): the number of ranges of identifiers.  Then the
// structure for reading the parse's BWT by row: the run heads in position order, Elias-Fano starts plus a symbol
// per run.  Sizes in bytes.
#include "parse_text.hpp"
#include <fstream>
using namespace katka;

static int bits_for(u64 x) { int b = 1; while ((1ull << b) <= x) ++b; return b; }
static u64 ef_bits(u64 n, u64 u) {                       // Elias-Fano: n values below u
    if (!n) return 0;
    const int l = u > n ? bits_for(u / n) - 1 : 0;
    return n * (u64)l + n + (u >> l) + 1;
}

int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: textless-sizes PREFIX.lvl PREFIX.ptxt RANGES.tsv\n"); return 1; }
    setenv("KATKA_NO_FIRST_RUN", "1", 1);
    two_level L; L.load(argv[1]);
    auto P = [](const char *k, u64 v) { printf("%s\t%lu\n", k, v); };
    P("m_phrases", L.m); P("sigma", L.sigma); P("pairs", L.nblocks); P("pair_symbols", L.nsym);
    P("pbwt_runs", L.pbwt.nruns);
    P("lvl_B", L.B.bytes()); P("lvl_pbwt", L.pbwt.bytes()); P("lvl_dict", L.dict.bytes());
    if (L.has_blocks) {
        P("lvl_succ", L.succp.bytes()); P("lvl_ccont", L.ccont.size() * 8); P("lvl_BS", L.BS.bytes());
        P("lvl_bbwt", L.bbwt.bytes()); P("lvl_dstart", L.dstart.bytes()); P("lvl_jv", L.jv.bytes()); P("lvl_sidev", L.sidev.size() * 8);
    }
    P("lvl_parse", L.bytes_parse()); P("lvl_blocks", L.has_blocks ? L.bytes_blocks() : 0);
    parse_text PT; PT.load(argv[2]);
    if (PT.sigma != L.sigma) die("the .ptxt and the .lvl differ in sigma");
    const u64 K = PT.K, sigma = PT.sigma, total = PT.dict.total;
    u64 R = 0;
    { std::ifstream f(argv[3]); std::string k; u64 v; while (f >> k >> v) if (k == "ranges") R = v; }
    if (!R) die("no ranges in " + std::string(argv[3]));
    const u64 others = PT.dict.marked.bytes() + PT.dict.roff.bytes() + PT.dict.rpos.bytes() + PT.dict.rlen.bytes() + PT.dict.dpos.bytes();
    P("phrases_marked", PT.dict.nmarked); P("bases_whole", total); P("ranges", R);
    P("other_chars", others);
    auto copy = [&](const char *name, u64 bases) {       // packed bases + offsets (sigma + 1 values below bases + 1)
        const u64 b = (bases * 2 + 7) / 8, o = (ef_bits(sigma + 1, bases + 1) + 7) / 8;
        printf("copy_%s_bases\t%lu\ncopy_%s_offsets\t%lu\ncopy_%s_total\t%lu\n", name, b, name, o, name, b + o + others);
        return b + o + others;
    };
    copy("whole", total);
    copy("no_first_k", total - K * sigma);
    copy("no_last_k", total - K * sigma);
    // one copy for both ends: each phrase without its first k, plus each range's k-mer once (the first k characters of
    // phrase y are its range's k-mer, found by rank on the range-start bits)
    const u64 both = copy("no_first_k_plus_range_kmers", total - K * sigma + K * R);
    (void)both;
    const u64 ptr = (sigma * (u64)bits_for(R) + 7) / 8, rbits = (sigma + 7) / 8;
    P("ranges_pointer_per_phrase", ptr); P("ranges_start_bits", rbits); P("ranges_total", ptr + rbits);
    const u64 rh = (ef_bits(L.pbwt.nruns, L.m + 1) + L.pbwt.nruns * (u64)bits_for(sigma) + 7) / 8;
    P("pbwt_by_row", rh);
    return 0;
}
