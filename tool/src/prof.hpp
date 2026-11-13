// Profiling STPD over the parse (2026-11-12, "faster STPD of the parse", item 1).  Compiled in only with
// -DKATKA_PROF (experiments/stpd_parse_profile.sh builds bin/katka-prof); bin/katka is unchanged.  Single-threaded.
//
// Time split, in TSC cycles (converted with a calibration against steady_clock):
//   0 right end   character steps from the window's end back to its last syncmer start (RLCSA);
//   1 enter       entering the parse from the right end (B rank, the id's block, entry[]);
//   2 binsearch   parse_stpd::find (the sample's binary search, including reading phrase characters);
//   3 extension   the parse loop without find: comparing Z[u-1] with the pattern's phrases, right to left;
//   4 left end    character-level STPD steps (stpd_extend) after the parse, or between parse stretches;
//   5 move        leaving the parse (Z's start of the occurrence), and re-entering it (pred, start, lower_bound);
//   6 parse       parsing each segment and its reverse complement into phrases (parse_pattern).
// Everything else is the run's total less these.
//
// LCE queries (records written to $KATKA_PROF_OUT.queries, the patterns' phrase ids to $KATKA_PROF_OUT.patterns):
//   kind 0 probe     one comparison of a binary search: a forward LCE, the longest common prefix of the pattern's
//                    pid[p..p+limit) and Z[x..], starting after skip phrases already known to match; answer = lcp.
//                    (If answer = limit, the probe goes on to compare characters of the next phrase.)
//   kind 1 extension a maximal run of the parse loop's comparisons: a backward LCE, the longest common suffix of
//                    pid[..p] and Z[..x] (x = UINT64_MAX if the occurrence starts Z); answer = phrases matched;
//                    flags 1: ended by a mismatch (exact LCE), 0: by the end of what the search may read (limit).
#pragma once
#ifdef KATKA_PROF
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <string>
#include <vector>
#include <x86intrin.h>
namespace katka {
struct prof_query {                     // 48 bytes
    uint8_t kind, dir, flags, pad0; uint32_t segment;
    uint64_t search; int64_t p; uint64_t x; uint32_t skip, answer, limit, pad1;
};
struct ps_prof {
    static constexpr int NT = 22;
    uint64_t cyc[NT] = {};
    uint64_t searches[2] = {}, finds = 0, probes = 0, probe_cmp = 0, ext_runs = 0, ext_read = 0;
    // two-level search (search2): searches, those reaching the parse, and the character steps at each end
    uint64_t tl_searches[2] = {}, tl_parse[2] = {}, tl_right[2] = {}, tl_left[2] = {}, tl_charonly[2] = {}, tl_parse_steps[2] = {};
    uint32_t segment = 0; uint64_t search = 0; int dir = 0;
    FILE *q = nullptr, *pat = nullptr;
    uint64_t tsc0 = 0; std::chrono::steady_clock::time_point c0;
    ps_prof() {
        const char *o = getenv("KATKA_PROF_OUT");
        if (o) { q = fopen((std::string(o) + ".queries").c_str(), "wb"); pat = fopen((std::string(o) + ".patterns").c_str(), "wb"); }
        tsc0 = __rdtsc(); c0 = std::chrono::steady_clock::now();
    }
    double hz() const {
        double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - c0).count();
        return (double)(__rdtsc() - tsc0) / s;
    }
    static inline uint64_t now() { return __rdtsc(); }
    // a new segment: its phrase ids in both directions (pi: the read, qi: its reverse complement)
    void pattern(const std::vector<uint32_t> &pid0, const std::vector<uint32_t> &pid1) {
        ++segment;
        if (!pat) return;
        for (const auto *v : {&pid0, &pid1}) {
            uint64_t n = v->size(); fwrite(&segment, 4, 1, pat); fwrite(&n, 8, 1, pat); fwrite(v->data(), 4, n, pat);
        }
    }
    void put(int kind, int64_t p, uint64_t x, uint64_t skip, uint64_t answer, uint64_t limit, int flags) {
        if (!q) return;
        prof_query r{(uint8_t)kind, (uint8_t)dir, (uint8_t)flags, 0, segment, search, p, x, (uint32_t)skip, (uint32_t)answer, (uint32_t)limit, 0};
        fwrite(&r, sizeof r, 1, q);
    }
    int64_t probe_p = 0;                 // set by the caller of find: the pattern index of e[0]
    void probe(uint64_t b, uint64_t skip, uint64_t lcp, uint64_t h) {
        ++probes; probe_cmp += (lcp >= skip ? lcp - skip : 0) + 1;
        put(0, probe_p, b, skip, lcp, h, 0);
    }
    void print(FILE *f) {
        if (q) fclose(q), q = nullptr;
        if (pat) fclose(pat), pat = nullptr;
        const double H = hz();
        static const char *name[NT] = {"right end (character steps)", "entering the parse", "binary search", "extension (parse loop without binary search)",
                                       "left end (character-level STPD)", "leaving / re-entering the parse", "parsing the patterns (syncmers, dictionary lookups)",
                                       "read input (FASTA parsing)", "output (writing lines)", "all of finder::run (per read)", "reverse complement (prepare)",
                                       "parsing: closed-syncmer scan", "parsing: phrase hashes", "parsing: dictionary slots", "parsing: phrase ids", "parsing: finish (blocks, fingerprints)",
                                       "two-level: right-end character steps", "two-level: entering the parse (B rank)", "two-level: parse steps (with blocks or shortcuts)",
                                       "two-level: leaving the parse (B select)", "two-level: left-end character steps", "two-level: searches with character steps only"};
        if (searches[0] + searches[1])
            fprintf(f, "profile: TSC %.3f GHz; STPD searches %lu backward + %lu forward; binary searches %lu, probes %lu (phrase comparisons %lu); "
                       "extension runs %lu, phrases read %lu\n", H / 1e9, searches[0], searches[1], finds, probes, probe_cmp, ext_runs, ext_read);
        else fprintf(f, "profile: TSC %.3f GHz\n", H / 1e9);
        for (int d = 0; d < 2; ++d)
            if (tl_searches[d])
                fprintf(f, "profile two-level, %s searches: %lu searches, %lu reaching the parse; right-end character steps %lu, left-end %lu, "
                           "in searches without the parse %lu; parse steps %lu\n", d ? "forward" : "backward", tl_searches[d], tl_parse[d], tl_right[d], tl_left[d],
                        tl_charonly[d], tl_parse_steps[d]);
        double sum = 0;
        for (int t = 0; t < NT; ++t) {
            if (!cyc[t]) continue;
            fprintf(f, "profile time: %s %.3f s\n", name[t], cyc[t] / H);
            if (t != 9 && (t < 11 || t > 15)) sum += cyc[t] / H;      // (9 contains the others; 11-15 are inside 6)
        }
        fprintf(f, "profile time: sum of the parts %.3f s (everything else = the total less this)\n", sum);
    }
};
inline ps_prof g_prof;
struct prof_scope { int t; uint64_t s; prof_scope(int t_) : t(t_), s(__rdtsc()) {} ~prof_scope() { g_prof.cyc[t] += __rdtsc() - s; } };
}  // namespace katka
#define PROF_CAT2(a, b) a##b
#define PROF_CAT(a, b) PROF_CAT2(a, b)
#define PROF_SCOPE(t) katka::prof_scope PROF_CAT(prof_scope_, __LINE__)(t)
#define PROF(...) __VA_ARGS__
#else
#define PROF_SCOPE(t)
#define PROF(...)
#endif
