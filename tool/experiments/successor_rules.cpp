// Successor rules for blocks (2026-11-08: "blocks with the smallest or the largest successor,
// chosen per phrase").  From a two-level index (PREFIX.lvl), recovers the parse by inverting its parse BWT, then
// for four rules for the designated successor chi(p) of each distinct phrase p:
//   smallest   the smallest phrase that follows p (the paper's rule; the index's default)
//   largest    the largest phrase that follows p
//   perphrase  whichever of the two follows p more often (the smallest on a tie): one bit per phrase
//   frequent   the most frequent successor (the smallest on a tie; --continuation frequent)
//   unique     the successor of p only if p has exactly one distinct successor (unitig-like blocks), else none
// reports the fraction of phrase occurrences followed by chi of their phrase, z_D (blocks), sigma_D (distinct
// block symbols (d, j, side), keyed as twolevel_build.hpp keys them) and r_D (runs in the BWT of the block
// sequence, by libsais over the block symbols in the index's symbol order).  With --check-order it also checks
// that the block suffixes are in the same order as the parse suffixes at block starts (Lemma "blockorder" and
// "Other continuations"), for every rule, by the parse's suffix array.  Finally, among the distinct phrases
// with at least 3 distinct successors: how often the most frequent successor is the smallest, the largest, or
// neither, and the share of phrase occurrences those phrases have.
//
// Build, from tool/:
//   g++ -std=c++17 -O3 -march=native -DNDEBUG -Isrc -Ithird_party/libsais/include experiments/successor_rules.cpp
//       build/libsais.o build/libsais64.o -o bin/successor-rules -lz -pthread
// Run:
//   bin/successor-rules PREFIX [--check-order] > out.tsv
// Memory: 8 bytes per phrase while the parse is recovered and while successors are grouped, then 4 per phrase
// plus 4 to 8 per block per rule; with --check-order, 8 per phrase more at the end (the parse's suffix array).
#include "twolevel.hpp"
#include "libsais.h"
#include <algorithm>
using namespace katka;

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: successor-rules PREFIX [--check-order]\n"); return 1; }
    const std::string prefix = argv[1];
    const bool check_order = argc > 2 && std::string(argv[2]) == "--check-order";
    double t0 = now();
    two_level L; L.load(prefix + ".lvl");
    const uint64_t m = L.m, N = m + 1;
    const uint32_t sigma = L.sigma;
    const EFRank &W = L.pbwt;
    if (W.n != N || !W.slot.empty()) die("unexpected parse BWT layout");
    fprintf(stderr, "%s: m = %lu phrases, sigma = %u, parse BWT %lu runs, smallest-rule blocks %lu (sided %d, max block %u)\n",
            prefix.c_str(), m, sigma, W.nruns, L.nblocks, (int)L.sided, L.maxblock);

    // LF over the parse BWT from its runs; Fb marks the first F row of each character (all of 0..sigma occur)
    std::vector<uint32_t> LF(N), P(N);
    BitRank Fb; Fb.init(N);
    {
        uint64_t prevc = UINT64_MAX, nchars = 0;
        for (uint64_t j = 0; j < W.nruns; ++j) {
            uint64_t ks = W.KS.access(j), c = ks / N, start = ks % N, f = W.FP.access(j), fn = j + 1 < W.nruns ? W.FP.access(j + 1) : N;
            if (c != prevc) { Fb.set(f); prevc = c; ++nchars; }
            for (uint64_t t = 0; t < fn - f; ++t) LF[start + t] = (uint32_t)(f + t);
        }
        Fb.finish();
        if (nchars != (uint64_t)sigma + 1) die("not every phrase id occurs in the parse BWT");
    }
    // invert: row 0 is the suffix "0" (position m); P[k] holds F rows, then ids
    {
        uint64_t r = 0;
        for (uint64_t k = m; k > 0; --k) { uint32_t x = LF[r]; P[k - 1] = x; r = x; }
        if (Fb.rank1((uint64_t)LF[r] + 1) - 1 != 0) die("inversion did not end at the terminator");
    }
    std::vector<uint32_t>().swap(LF);
    for (uint64_t k = 0; k < m; ++k) P[k] = (uint32_t)(Fb.rank1((uint64_t)P[k] + 1) - 1);
    P[m] = 0;
    Fb = BitRank();
    for (uint64_t k = 0; k < m; ++k) if (P[k] == 0) die("id 0 inside the parse");
    fprintf(stderr, "parse recovered (%.1f s)\n", now() - t0);

    // successors grouped by phrase (occurrences i = 0..m-2 that have a successor)
    std::vector<uint64_t> off(sigma + 2, 0);
    for (uint64_t i = 0; i + 1 < m; ++i) off[P[i] + 1]++;
    for (uint32_t x = 0; x <= sigma; ++x) off[x + 1] += off[x];
    std::vector<uint32_t> smallest(sigma + 1, 0), largest(sigma + 1, 0), frequent(sigma + 1, 0), perphrase(sigma + 1, 0), unique(sigma + 1, 0);
    std::vector<uint64_t> occ(sigma + 1, 0), cs(sigma + 1, 0), cl(sigma + 1, 0), cf(sigma + 1, 0);
    std::vector<uint32_t> nsucc(sigma + 1, 0);
    std::vector<uint8_t> ftie(sigma + 1, 0);     // the maximum count is attained by more than one successor
    {
        std::vector<uint32_t> Y(m ? m - 1 : 0);
        {
            std::vector<uint64_t> pos(off.begin(), off.end() - 1);
            for (uint64_t i = 0; i + 1 < m; ++i) Y[pos[P[i]]++] = P[i + 1];
        }
        for (uint32_t x = 1; x <= sigma; ++x) {
            uint64_t a = off[x], b = off[x + 1];
            occ[x] = b - a;
            if (a == b) continue;
            std::sort(Y.begin() + a, Y.begin() + b);
            smallest[x] = Y[a]; largest[x] = Y[b - 1];
            uint64_t best = 0; uint32_t by = 0; bool tie = false;
            for (uint64_t i = a; i < b;) {
                uint64_t e = i + 1;
                while (e < b && Y[e] == Y[i]) ++e;
                uint64_t c = e - i;
                ++nsucc[x];
                if (Y[i] == smallest[x]) cs[x] = c;
                if (Y[i] == largest[x]) cl[x] = c;
                if (c > best) { best = c; by = Y[i]; tie = false; } else if (c == best) tie = true;   // ascending: ties keep the smallest
                i = e;
            }
            frequent[x] = by; cf[x] = best; ftie[x] = tie;
            perphrase[x] = cl[x] > cs[x] ? largest[x] : smallest[x];
            unique[x] = nsucc[x] == 1 ? smallest[x] : 0;
        }
    }
    // the index's own successors (this .lvl's rule) must be what we recompute
    {
        const std::vector<uint32_t> &mine = L.sided ? frequent : smallest;
        if (L.has_blocks && !L.maxblock) for (uint32_t x = 0; x <= sigma; ++x) if (L.succ(x) != mine[x]) die("successor of " + std::to_string(x) + " differs from the index's");
    }
    fprintf(stderr, "successors (%.1f s)\n", now() - t0);

    struct rule { const char *name; const std::vector<uint32_t> *succ; uint64_t followed = 0, z = 0, nsym = 0, rbwt = 0; std::vector<uint32_t> order; };
    std::vector<rule> R = {{"smallest", &smallest}, {"largest", &largest}, {"perphrase", &perphrase}, {"frequent", &frequent}, {"unique", &unique}};
    for (auto &ru : R) {
        const std::vector<uint32_t> &su = *ru.succ;
        const bool all_plus = ru.succ == &smallest || ru.succ == &unique;   // as twolevel_build.hpp: only + (unique: a block
                                                              // ends only at a phrase with no chi)
        std::vector<uint64_t> keys;                           // one per block, in text order
        std::vector<uint32_t> bstart;                         // block starts (parse positions)
        uint64_t start = 0;
        for (uint64_t i = 1; i <= m; ++i) {
            if (i < m && su[P[i - 1]] == P[i]) { ++ru.followed; continue; }
            uint64_t j = i - 1 - start;
            bool plus = all_plus || (i < m && P[i] > su[P[i - 1]]);
            keys.push_back(((uint64_t)P[start] << 33) | two_level::sym_key(j, plus));
            bstart.push_back((uint32_t)start);
            start = i;
        }
        ru.z = keys.size();
        std::vector<uint64_t> u(keys);
        std::sort(u.begin(), u.end()); u.erase(std::unique(u.begin(), u.end()), u.end());
        ru.nsym = u.size();
        std::vector<int32_t> T(ru.z + 1), SA(ru.z + 1);
        for (uint64_t b = 0; b < ru.z; ++b) T[b] = (int32_t)(std::lower_bound(u.begin(), u.end(), keys[b]) - u.begin()) + 1;
        T[ru.z] = 0;
        std::vector<uint64_t>().swap(keys); std::vector<uint64_t>().swap(u);
        std::vector<int32_t> T2(T);
        if (libsais_int(T2.data(), SA.data(), (int32_t)(ru.z + 1), (int32_t)ru.nsym + 1, 0) != 0) die("libsais_int failed");
        std::vector<int32_t>().swap(T2);
        int64_t prev = -1;
        for (uint64_t r = 0; r <= ru.z; ++r) {
            int64_t c = SA[r] == 0 ? -2 : T[SA[r] - 1];       // the BWT's terminator: the row of block 0
            if (c != prev) { ++ru.rbwt; prev = c; }
        }
        if (check_order) {                                    // block suffixes in block-SA order, as parse positions
            ru.order.resize(ru.z + 1);
            for (uint64_t r = 0; r <= ru.z; ++r) ru.order[r] = (uint64_t)SA[r] == ru.z ? (uint32_t)m : bstart[SA[r]];
        }
        if (ru.followed != (m - 1) - (ru.z - 1)) die("block count mismatch");
        fprintf(stderr, "%s: z = %lu, sigma = %lu, r = %lu (%.1f s)\n", ru.name, ru.z, ru.nsym, ru.rbwt, now() - t0);
    }
    // the index's own counts for its rule
    {
        const rule &mine = L.sided ? R[3] : R[0];
        if (L.has_blocks && !L.maxblock && (mine.z != L.nblocks || mine.nsym != L.nsym))
            die("the index's rule gives z = " + std::to_string(L.nblocks) + ", sigma = " + std::to_string(L.nsym));
        fprintf(stderr, "the index's rule (%s) agrees: z = %lu, sigma = %lu\n", mine.name, mine.z, mine.nsym);
    }

    // phrases with at least 3 distinct successors
    uint64_t n3 = 0, o3 = 0, f_s = 0, f_l = 0, f_n = 0, ties = 0, s_max = 0, l_max = 0, occall = m - 1;
    for (uint32_t x = 1; x <= sigma; ++x) {
        if (nsucc[x] < 3) continue;
        ++n3; o3 += occ[x];
        if (frequent[x] == smallest[x]) ++f_s; else if (frequent[x] == largest[x]) ++f_l; else ++f_n;
        ties += ftie[x];
        s_max += cs[x] == cf[x]; l_max += cl[x] == cf[x];
    }
    uint64_t n2 = 0; for (uint32_t x = 1; x <= sigma; ++x) n2 += nsucc[x] >= 2;

    // the parse's suffix array: the block suffixes must be in the same order as the parse suffixes at block starts
    std::vector<int> order_ok(R.size(), -1);
    if (check_order) {
        std::vector<std::vector<uint8_t>> isb(R.size());
        for (size_t q = 0; q < R.size(); ++q) {
            isb[q].assign(N / 8 + 1, 0);
            for (uint32_t p : R[q].order) isb[q][p >> 3] |= 1 << (p & 7);
        }
        std::vector<uint32_t> SA(N);
        if (libsais_int(reinterpret_cast<int32_t *>(P.data()), reinterpret_cast<int32_t *>(SA.data()), (int32_t)N, (int32_t)sigma + 1, 0) != 0)
            die("libsais_int failed");
        std::vector<uint32_t>().swap(P);
        for (size_t q = 0; q < R.size(); ++q) {
            uint64_t k = 0; bool ok = true;
            for (uint64_t r = 0; r < N && ok; ++r) {
                uint32_t p = SA[r];
                if (!((isb[q][p >> 3] >> (p & 7)) & 1)) continue;
                ok = k < R[q].order.size() && R[q].order[k] == p; ++k;
            }
            order_ok[q] = ok && k == R[q].order.size();
            fprintf(stderr, "%s: block order %s the parse order\n", R[q].name, order_ok[q] ? "equals" : "DIFFERS FROM");
        }
    }

    printf("prefix\t%s\nphrases\t%lu\ndistinct\t%u\noccurrences_with_successor\t%lu\n", prefix.c_str(), m, sigma, occall);
    printf("rule\tfollowed\tfraction\tblocks\tsymbols\tblock_bwt_runs\tblock_order_equals_parse_order\n");
    for (size_t q = 0; q < R.size(); ++q)
        printf("%s\t%lu\t%.6f\t%lu\t%lu\t%lu\t%s\n", R[q].name, R[q].followed, (double)R[q].followed / occall, R[q].z, R[q].nsym, R[q].rbwt,
               order_ok[q] < 0 ? "unchecked" : order_ok[q] ? "yes" : "NO");
    printf("distinct_with_ge2_successors\t%lu\n", n2);
    printf("distinct_with_ge3_successors\t%lu\n", n3);
    printf("their_occurrences\t%lu\t%.6f\n", o3, (double)o3 / occall);
    printf("ge3_frequent_is_smallest\t%lu\t%.6f\n", f_s, n3 ? (double)f_s / n3 : 0.0);
    printf("ge3_frequent_is_largest\t%lu\t%.6f\n", f_l, n3 ? (double)f_l / n3 : 0.0);
    printf("ge3_frequent_is_neither\t%lu\t%.6f\n", f_n, n3 ? (double)f_n / n3 : 0.0);
    printf("ge3_max_count_tied\t%lu\t%.6f\n", ties, n3 ? (double)ties / n3 : 0.0);
    printf("ge3_smallest_attains_max\t%lu\t%.6f\n", s_max, n3 ? (double)s_max / n3 : 0.0);
    printf("ge3_largest_attains_max\t%lu\t%.6f\n", l_max, n3 ? (double)l_max / n3 : 0.0);
    fprintf(stderr, "done (%.1f s)\n", now() - t0);
    return 0;
}
