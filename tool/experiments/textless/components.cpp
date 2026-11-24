// Two-level search without the index of the text: the space of the built minimal-width index by component, and a few
// facts about its phrases and ranges.
//   bin/textless-components PREFIX [PREFIX1.tlx]   (reads PREFIX.lvl and PREFIX.tl2, as find --textless does; the .tlx,
//   with the same phrases whole, only for the text's first phrase)
// Sizes in bytes, as loaded, without the first-run table.  Also: the longest phrase over A/C/G/T and the longest with
// other characters (X, $), in characters; phrase 1 (the text's first) and whether it starts with a closed syncmer;
// the largest range of identifiers (the most phrases one binary search at the right end can cover).
// Build, from tool/:
//   g++ -std=c++17 -O3 -march=native -DNDEBUG -Isrc -Ithird_party/libsais/include experiments/textless/components.cpp
//       build/libsais.o build/libsais64.o -o bin/textless-components -lz -pthread
#include "textless.hpp"
using namespace katka;

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: textless-components PREFIX [TLX]\n"); return 1; }
    const std::string prefix = argv[1];
    setenv("KATKA_NO_FIRST_RUN", "1", 1);
    two_level L; L.load(prefix + ".lvl");
    textless_index2 T; T.load(prefix + ".tl2");
    auto P = [](const char *k, u64 v) { printf("%s\t%lu\n", k, v); };
    P("sigma", T.sigma); P("ranges", T.R); P("parse_rows", T.N); P("pbwt_runs", L.pbwt.nruns);
    P("B", L.B.bytes()); P("has_blocks", L.has_blocks);
    P("parse_bwt", L.pbwt.bytes());
    P("dictionary", L.dict.bytes());
    P("copies_bases", (T.cr.size() + T.cl.size()) * 8);
    P("copies_offsets", T.coff.bytes());
    P("whole_phrases", T.marked.bytes() + T.other.size() + T.ooff.bytes());
    P("ranges_bits", T.rbits.bytes());
    P("ranges_lastr", T.lastr.bytes());
    P("run_heads_starts", T.hstart.bytes());
    P("run_heads_symbols", T.hsym.bytes());
    P("tl2_total", T.bytes());
    u64 lacgt = 0, lother = 0, yacgt = 0, yother = 0, nother = 0, lsum = 0, gap = 0, nshort = 0;
    for (u64 y = 1; y <= T.sigma; ++y) {
        const u64 l = T.len(y);
        if (T.marked.get(y)) { ++nother; if (l > lother) { lother = l; yother = y; } }
        else {
            if (l > lacgt) { lacgt = l; yacgt = y; }
            lsum += l; if (l < 2 * T.K) { gap += 2 * T.K - l; ++nshort; }   // y[|y| - K, K): in neither copy
        }
    }
    P("longest_acgt", lacgt); P("longest_acgt_id", yacgt);
    P("acgt_bases_whole", lsum); P("acgt_shorter_than_2K", nshort); P("acgt_bases_in_neither_copy", gap);
    P("phrases_other", nother); P("longest_other", lother); P("longest_other_id", yother);
    u64 rmax = 0, rmaxr = 0;
    for (u64 r = 0; r < T.R; ++r) { u64 a, b; T.range(r, a, b); if (b - a > rmax) { rmax = b - a; rmaxr = r; } }
    P("largest_range", rmax); P("largest_range_index", rmaxr);
    // phrase 1 is not necessarily the text's first phrase (ids are in lexicographic order): find the phrase whose
    // parse row is preceded by 0, the parse's end (the row of the parse suffix that is the whole parse)
    u64 first = 0;
    for (u64 jr = 0; jr < T.hruns; ++jr) if (T.hsym.get(jr) == 0) {
        const u64 row = T.hstart.access(jr);
        for (u64 y = 1; y <= T.sigma; ++y) if (tl_first_row(L.pbwt, y + 1) > row) { first = y; break; }
        break;
    }
    P("first_phrase_id", first);
    if (first && argc > 2) {                             // whole, from the fixed-width index (the short copies do not cover
        textless_index T1; T1.load(argv[2]);              // [|y| - K, K) of a phrase shorter than 2K)
        const u64 l = T1.len(first);
        std::string s(l, '?'); T1.chars(first, 0, l, (uint8_t *)&s[0]);
        P("first_phrase_len", l); P("first_phrase_marked", T.marked.get(first));
        bool sync = false;
        for_each_syncmer((const uint8_t *)s.data(), s.size(), L.K, L.SM, [&](uint64_t q) { if (q == 0) sync = true; });
        P("first_phrase_starts_with_syncmer", sync);
        u64 a = 0, b = 0;
        for (u64 r = 0; r < T.R; ++r) { T.range(r, a, b); if (first >= a && first < b) break; }
        P("first_phrase_range_size", b - a);
        printf("first_phrase_prefix\t%s\n", s.substr(0, std::min<size_t>(s.size(), 80)).c_str());
    }
    return 0;
}
