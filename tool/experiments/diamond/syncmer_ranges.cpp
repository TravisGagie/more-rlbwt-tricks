// The phrases that start with the same syncmer (the first K characters) have
// consecutive ids, because the ids are the phrases' lexicographic order.  From the parse oracle's dictionary
// (PREFIX.ptxt: the distinct phrases in id order), this reports
//   - the check: the ids are in strictly increasing lexicographic order, and no first K-mer occurs in two
//     separate runs of ids;
//   - the ranges (runs of ids with the same first K-mer): how many, and their sizes, by range and by phrase;
//   - for each search of a dump from bin/katka-dia ($KATKA_DIACOUNT_ALPHA: alpha and its right-end case per
//     search), the size of the range of phrases that start with alpha's last K-mer, so a binary search inside it
//     takes ceil(log2(size)) comparisons;
//   - sizes: the phrases without their first K characters at 2 bits per base, their offsets, a pointer per phrase
//     to its last K-mer's range, and a bit per phrase marking the ranges' starts.
//
// Build, from tool/:
//   g++ -std=c++17 -O3 -march=native -DNDEBUG -Isrc -Ithird_party/libsais/include experiments/diamond/syncmer_ranges.cpp
//       build/libsais.o build/libsais64.o -o bin/syncmer-ranges -lz -pthread
// Run:
//   bin/syncmer-ranges PREFIX.ptxt [ALPHA_DUMP ...] > out.tsv
#include "parse_text.hpp"
#include <unordered_map>
#include <cmath>
using namespace katka;

static int bits_for(u64 x) { int b = 1; while ((1ull << b) <= x) ++b; return b; }

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: syncmer-ranges PREFIX.ptxt [ALPHA_DUMP ...]\n"); return 1; }
    parse_text PT; PT.load(argv[1]);
    const u64 K = PT.K, sigma = PT.sigma;
    fprintf(stderr, "%s: sigma = %lu, K = %lu\n", argv[1], sigma, K);
    std::vector<uint8_t> b1, b2;
    std::unordered_map<std::string, uint32_t> first;      // first K-mer -> its range (0-based)
    std::vector<uint64_t> rstart;                         // range r is ids rstart[r] .. rstart[r + 1] - 1
    std::vector<uint32_t> lastr(sigma + 1, UINT32_MAX);   // the range of alpha's last K-mer (UINT32_MAX: none)
    std::string prev, cur;
    u64 order_bad = 0, split = 0, short_ph = 0, bases_past_K = 0, total = 0;
    for (u64 id = 1; id <= sigma; ++id) {
        const u64 len = PT.len_of(id); total += len;
        const uint8_t *p = PT.phrase(id, 0, len, b1);
        if (id > 1) {                                     // strictly increasing, byte order
            const u64 l = std::min<u64>(len, prev.size());
            const int c = memcmp(prev.data(), p, l);
            if (c > 0 || (c == 0 && prev.size() >= len)) ++order_bad;
        }
        prev.assign((const char *)p, len);
        if (len < K) { ++short_ph; continue; }
        bases_past_K += len - K;
        cur.assign((const char *)p, K);
        auto it = first.find(cur);
        if (it == first.end()) { first.emplace(cur, (uint32_t)rstart.size()); rstart.push_back(id); }
        else if (it->second + 1 != rstart.size()) ++split;   // the K-mer came back after another one
    }
    rstart.push_back(sigma + 1);
    for (u64 id = 1; id <= sigma; ++id) {
        const u64 len = PT.len_of(id);
        if (len < K) continue;
        const uint8_t *p = PT.phrase(id, len - K, len, b2);
        auto it = first.find(std::string((const char *)p, K));
        if (it != first.end()) lastr[id] = it->second;
    }
    const u64 R = rstart.size() - 1;
    printf("sigma\t%lu\nK\t%lu\nranges\t%lu\norder_violations\t%lu\nsplit_ranges\t%lu\nphrases_shorter_than_K\t%lu\n", sigma, K, R, order_bad, split, short_ph);
    std::vector<u64> sz(R);
    for (u64 r = 0; r < R; ++r) sz[r] = rstart[r + 1] - rstart[r];
    auto pct = [](std::vector<u64> v, double q) { std::sort(v.begin(), v.end()); return v.empty() ? 0 : v[std::min<u64>(v.size() - 1, (u64)(q * v.size()))]; };
    printf("range_size_mean\t%.2f\n", (double)(sigma - short_ph) / R);
    for (double q : {0.5, 0.9, 0.99}) printf("range_size_p%02d_by_range\t%lu\n", (int)(q * 100), pct(sz, q));
    { std::vector<u64> byph; byph.reserve(sigma); for (u64 r = 0; r < R; ++r) for (u64 t = 0; t < sz[r]; ++t) byph.push_back(sz[r]);
      for (double q : {0.5, 0.9, 0.99}) printf("range_size_p%02d_by_phrase\t%lu\n", (int)(q * 100), pct(byph, q)); }
    printf("range_size_max\t%lu\n", *std::max_element(sz.begin(), sz.end()));
    // sizes (bytes): the copy without the first K, its offsets, the pointer per phrase, the bit per phrase
    const u64 copy = (bases_past_K * 2 + 7) / 8, offs = (sigma * (u64)bits_for(bases_past_K) + 7) / 8,
              ptr = (sigma * (u64)bits_for(R) + 7) / 8, bv = (sigma + 7) / 8;
    printf("bases_total\t%lu\nbases_past_K\t%lu\nbytes_copy_past_K\t%lu\nbytes_offsets\t%lu\nbytes_pointer_per_phrase\t%lu\nbytes_range_start_bits\t%lu\n",
           total, bases_past_K, copy, offs, ptr, bv);
    for (int a = 2; a < argc; ++a) {                      // per search: alpha's last K-mer's range
        FILE *f = open_or_die(argv[a], "rb");
        uint32_t v[2];
        std::vector<u64> s[2]; u64 none = 0;
        while (fread(v, 4, 2, f) == 2) {
            const uint32_t c = v[1] & 7;
            if (c != 2 && c != 3) continue;
            if (lastr[v[0]] == UINT32_MAX) { ++none; continue; }
            s[c - 2].push_back(sz[lastr[v[0]]]);
        }
        fclose(f);
        for (int c = 0; c < 2; ++c) {
            const char *nm = c ? "s_starts" : "no_successor";
            double lg = 0; for (u64 x : s[c]) lg += x > 1 ? std::ceil(std::log2((double)x)) : 0;
            printf("%s\t%s_searches\t%zu\n", argv[a], nm, s[c].size());
            for (double q : {0.5, 0.9, 0.99}) printf("%s\t%s_range_p%02d\t%lu\n", argv[a], nm, (int)(q * 100), pct(s[c], q));
            printf("%s\t%s_mean_comparisons\t%.2f\n", argv[a], nm, s[c].empty() ? 0.0 : lg / s[c].size());
        }
        printf("%s\tno_range\t%lu\n", argv[a], none);
    }
    return 0;
}
