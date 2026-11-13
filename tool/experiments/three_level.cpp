// Would a third level help? (2026-11-09.)  A measurement, not an implementation.
//
// From a two-level index (PREFIX.lvl: closed syncmers k = 51, s = 12 over characters), recovers the parse P (phrase
// ids) by inverting its parse BWT and parses P again with closed syncmers over phrase ids, for several (k2, s2):
//   - each id x gets a 64-bit hash h(x); an s2-mer of ids gets the hash of its s2 hashes;
//   - a window of k2 ids starting at t is a closed syncmer when its smallest s2-mer hash (the leftmost on a tie) is
//     its first or its last s2-mer; t is then a level-2 trigger;
//   - the level-2 phrase from trigger t_j is P[t_j .. t_{j+1} + k2), so consecutive level-2 phrases overlap by k2
//     ids, as level-1 phrases overlap by k characters: a level-2 phrase is then determined by its own ids.
// Reports, per setting: level-2 phrases (triggers), distinct ones (64-bit fingerprints of P[t_j .. t_{j+1} + k2)),
// level-1 phrases per level-2 phrase (m / triggers, the step between triggers), and the blocks of the index.
//
// Then, for the MEMs of length >= L in MEMS.tsv (name, start, end; the paper's, from find --format mems) of the
// patterns in Q.fa: the pattern's level-1 parse (as find parses it), the whole level-1 phrases inside each MEM
// (P[S_i .. S_{i+1} + k) inside [start, end)), and within those:
//   - blocks: the block steps over them with the index's continuation rule (one per block start: a phrase that
//     doesn't continue its predecessor's block, or the first);
//   - level 2: the level-2 phrases whose whole extent P[t_j .. t_{j+1} + k2) lies inside, with the triggers found
//     from the windows inside the MEM only (the same as in the text, since syncmers are local); the run of them
//     from the first such t_j to the last t_{j+1} is crossed by level-2 steps, one each;
//   - fringes: the level-1 phrases before the first whole level-2 phrase and after the last one's core
//     (from its t_{j+1} on): these need level-1 steps (or block steps: also counted).
//   So a three-level search would cross a MEM's whole phrases with (level-2 phrases) + (fringe phrases) steps, or
//   (level-2 phrases) + (fringe blocks) with blocks in the fringes, against (blocks) steps with blocks alone.
//   Characters outside whole level-1 phrases are the same for every method and aren't counted.
//
// Build, from tool/:
//   g++ -std=c++17 -O3 -march=native -DNDEBUG -Isrc -Ithird_party/libsais/include experiments/three_level.cpp
//       build/libsais.o build/libsais64.o -o bin/three-level -lz -pthread
// Run:
//   bin/three-level PREFIX Q.fa MEMS.tsv [--check] [k2,s2 ...] > out.tsv      (default 5,2 9,2 17,2 33,2 65,2)
// --check: every whole level-2 phrase found inside a MEM must be one of the text's level-2 phrases (by fingerprint;
// keeps 8 bytes per level-2 phrase of every setting).
// Memory: 8 bytes per phrase while the parse is recovered, then 4 per phrase plus 8 per level-2 phrase.
#include "twolevel.hpp"
#include "twolevel_build.hpp"
#include <algorithm>
#include <deque>
#include <fstream>
#include <map>
using namespace katka;

static inline uint64_t mix(uint64_t x) {
    x += 0x9e3779b97f4a7c15ull; x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull; x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
    return x ^ (x >> 31);
}
static inline uint64_t hid(uint32_t x) { return mix((uint64_t)x * 0x2545f4914f6cdd1dull + 7); }

// level-2 triggers of ids[0 .. n): positions t with t + k2 <= n whose window is a closed syncmer
template <class Get> static void triggers(uint64_t n, int k2, int s2, Get get, std::vector<uint64_t> &out) {
    out.clear();
    if (n < (uint64_t)k2) return;
    const uint64_t ns = n - s2 + 1, w = k2 - s2 + 1;   // s2-mers, and s2-mers per window
    std::deque<std::pair<uint64_t, uint64_t>> dq;        // (hash, position), increasing hashes
    std::vector<uint64_t> hs(s2);
    auto smer = [&](uint64_t p) { uint64_t h = 0; for (int a = 0; a < s2; ++a) h = mix(h ^ hid(get(p + a))); return h; };
    for (uint64_t p = 0; p < ns; ++p) {
        uint64_t h = smer(p);
        while (!dq.empty() && dq.back().first > h) dq.pop_back();   // keeps the leftmost of equal minima
        dq.emplace_back(h, p);
        if (p + 1 >= w) {
            uint64_t t = p + 1 - w;
            while (dq.front().second < t) dq.pop_front();
            uint64_t q = dq.front().second;
            if (q == t || q == p) out.push_back(t);
        }
    }
}

struct fasta { std::vector<std::string> name, seq; };
static fasta read_fasta(const std::string &f) {
    fasta F; std::ifstream in(f); std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        if (line[0] == '>') { F.name.push_back(line.substr(1, line.find_first_of(" \t") - 1)); F.seq.emplace_back(); }
        else F.seq.back() += line;
    }
    return F;
}

static double median(std::vector<double> v) {
    if (v.empty()) return 0;
    std::sort(v.begin(), v.end());
    size_t h = v.size() / 2;
    return v.size() % 2 ? v[h] : (v[h - 1] + v[h]) / 2;
}

int main(int argc, char **argv) {
    if (argc < 4) { fprintf(stderr, "usage: three-level PREFIX Q.fa MEMS.tsv [k2,s2 ...]\n"); return 1; }
    const std::string prefix = argv[1];
    std::vector<std::pair<int, int>> settings;
    bool check = false;
    for (int a = 4; a < argc; ++a) { if (std::string(argv[a]) == "--check") { check = true; continue; } int k, s; if (sscanf(argv[a], "%d,%d", &k, &s) != 2 || s < 1 || k <= s) die("k2,s2 with k2 > s2 >= 1"); settings.push_back({k, s}); }
    if (settings.empty()) settings = {{5, 2}, {9, 2}, {17, 2}, {33, 2}, {65, 2}};
    double t0 = now();
    two_level L; L.load(prefix + ".lvl");
    const uint64_t m = L.m;
    std::vector<uint32_t> P = parse_from_pbwt(L);         // P[0 .. m), P[m] = 0
    fprintf(stderr, "%s: m = %lu phrases, sigma = %u, blocks %lu (%.1f s)\n", prefix.c_str(), m, L.sigma, L.nblocks, now() - t0);
    printf("prefix\t%s\nphrases\t%lu\ndistinct\t%u\nblocks\t%lu\nphrases_per_block\t%.3f\n", prefix.c_str(), m, L.sigma, L.nblocks, (double)m / L.nblocks);

    // level 2 over the text's parse
    printf("\nk2\ts2\tlevel2_phrases\tdistinct\tphrases_per_level2_phrase\n");
    std::vector<uint64_t> T, fps;
    std::vector<std::vector<uint64_t>> keep(settings.size());
    for (size_t si = 0; si < settings.size(); ++si) {
        auto [k2, s2] = settings[si];
        triggers(m, k2, s2, [&](uint64_t p) { return P[p]; }, T);
        fps.clear(); fps.reserve(T.size());
        for (size_t j = 0; j + 1 < T.size(); ++j) {
            uint64_t h = 0x12345;
            for (uint64_t p = T[j]; p < T[j + 1] + k2 && p < m; ++p) h = mix(h ^ hid(P[p]));
            fps.push_back(h);
        }
        std::sort(fps.begin(), fps.end());
        uint64_t d = std::unique(fps.begin(), fps.end()) - fps.begin();
        if (check) keep[si].assign(fps.begin(), fps.begin() + d);
        printf("%d\t%d\t%lu\t%lu\t%.3f\n", k2, s2, (uint64_t)T.size(), d, T.empty() ? 0.0 : (double)m / T.size());
        fflush(stdout);
        fprintf(stderr, "  k2 = %d, s2 = %d: %lu level-2 phrases, %lu distinct (%.1f s)\n", k2, s2, (uint64_t)T.size(), d, now() - t0);
    }
    std::vector<uint64_t>().swap(fps); std::vector<uint32_t>().swap(P);

    // the MEMs
    fasta Q = read_fasta(argv[2]);
    std::map<std::string, size_t> qi;
    for (size_t i = 0; i < Q.name.size(); ++i) qi[Q.name[i]] = i;
    std::vector<std::vector<std::pair<int64_t, int64_t>>> mems(Q.name.size());
    { std::ifstream in(argv[3]); std::string line;
      while (std::getline(in, line)) {
          std::string nm = line.substr(0, line.find('\t')); int64_t a, e;
          if (sscanf(line.c_str() + nm.size(), "%ld %ld", &a, &e) != 2) die("bad MEM line: " + line);
          auto it = qi.find(nm); if (it == qi.end()) die("MEM of an unknown pattern " + nm);
          mems[it->second].push_back({a, e});
      } }
    const int64_t K = L.K;
    std::vector<double> nph, nblk;
    struct acc { std::vector<double> whole, fr, frb, l2steps, l2bsteps; uint64_t nonzero = 0; };
    std::vector<acc> A(settings.size());
    uint64_t nmem = 0, zero_pid = 0, checked = 0, missing = 0;
    for (size_t q = 0; q < Q.name.size(); ++q) {
        const std::string &s = Q.seq[q];
        pattern_parse pi, qq;
        parse_pattern(L, s.data(), s.size(), pi, qq);
        const auto &S = pi.S;
        for (auto [a, e] : mems[q]) {
            ++nmem;
            // whole level-1 phrases i in [f, l]: S[i] >= a, S[i+1] + K <= e
            size_t f = std::lower_bound(S.begin(), S.end(), a) - S.begin();
            size_t l = f;                                  // one past the last
            while (l + 1 < S.size() && S[l + 1] + K <= e) ++l;
            const uint64_t n = l - f;
            nph.push_back((double)n);
            uint64_t blocks = 0;
            std::vector<uint64_t> bs;                      // block starts (relative)
            for (size_t i = f; i < l; ++i) {
                if (!pi.pid[i]) ++zero_pid;
                if (i == f || !pi.cont[i]) { ++blocks; bs.push_back(i - f); }
            }
            nblk.push_back((double)blocks);
            auto blocks_in = [&](uint64_t x, uint64_t y) {   // blocks needed for relative phrases [x, y)
                if (x >= y) return (uint64_t)0;
                uint64_t c = 1;                              // the one containing x, cut at x
                for (auto b : bs) if (b > x && b < y) ++c;
                return c;
            };
            for (size_t si = 0; si < settings.size(); ++si) {
                auto [k2, s2] = settings[si];
                std::vector<uint64_t> T2;
                triggers(n, k2, s2, [&](uint64_t p) { return pi.pid[f + p]; }, T2);
                // whole level-2 phrases: consecutive triggers T2[j], T2[j+1] with T2[j+1] + k2 <= n
                uint64_t first = UINT64_MAX, last = 0, w = 0;
                for (size_t j = 0; j + 1 < T2.size(); ++j)
                    if (T2[j + 1] + k2 <= n) {
                        if (first == UINT64_MAX) first = T2[j];
                        last = T2[j + 1]; ++w;
                        if (check) {
                            uint64_t h = 0x12345;
                            for (uint64_t p = T2[j]; p < T2[j + 1] + k2; ++p) h = mix(h ^ hid(pi.pid[f + p]));
                            ++checked; missing += !std::binary_search(keep[si].begin(), keep[si].end(), h);
                        }
                    }
                uint64_t fr, frb;
                if (!w) { fr = n; frb = blocks; }
                else { fr = first + (n - last); frb = blocks_in(0, first) + blocks_in(last, n); }
                A[si].whole.push_back((double)w); A[si].fr.push_back((double)fr); A[si].frb.push_back((double)frb);
                A[si].l2steps.push_back((double)(w + fr)); A[si].l2bsteps.push_back((double)(w + frb));
                A[si].nonzero += w > 0;
            }
        }
    }
    auto mean = [](const std::vector<double> &v) { double s = 0; for (double x : v) s += x; return v.empty() ? 0.0 : s / v.size(); };
    auto sum = [](const std::vector<double> &v) { double s = 0; for (double x : v) s += x; return s; };
    printf("\nMEMs\t%lu\nlevel1_phrases_per_MEM_mean\t%.2f\nlevel1_phrases_per_MEM_median\t%.1f\nlevel1_phrases_total\t%.0f\n"
           "blocks_per_MEM_mean\t%.2f\nblocks_per_MEM_median\t%.1f\nblock_steps_total\t%.0f\nphrases_not_in_dictionary\t%lu\n",
           nmem, mean(nph), median(nph), sum(nph), mean(nblk), median(nblk), sum(nblk), zero_pid);
    printf("\nk2\ts2\tMEMs_with_a_whole_level2_phrase\twhole_level2_mean\twhole_level2_median\tfringe_level1_mean\tfringe_level1_median"
           "\tfringe_blocks_mean\tsteps_level2+level1_total\tsteps_level2+blocks_total\tsteps_blocks_total\n");
    for (size_t si = 0; si < settings.size(); ++si) {
        auto &a = A[si];
        printf("%d\t%d\t%lu\t%.2f\t%.1f\t%.2f\t%.1f\t%.2f\t%.0f\t%.0f\t%.0f\n", settings[si].first, settings[si].second, a.nonzero,
               mean(a.whole), median(a.whole), mean(a.fr), median(a.fr), mean(a.frb), sum(a.l2steps), sum(a.l2bsteps), sum(nblk));
    }
    if (check) {
        printf("\ncheck: whole level-2 phrases in MEMs\t%lu\tnot a level-2 phrase of the text\t%lu\n", checked, missing);
        if (missing) die("level-2 phrases in MEMs that the text's level-2 parse doesn't have");
    }
    fprintf(stderr, "done (%.1f s)\n", now() - t0);
}
