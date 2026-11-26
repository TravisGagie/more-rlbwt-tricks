// Locating from the parse alone (2026-11-25), items 4 and 5: the parse's sizes, the samples that a phi^-1 over the
// parse would need, in phrase indices and in text positions, a check of phi^-1 in text positions over every row of
// the parse's suffix array, and the gaps between the parse run samples with the subsampling of Cobas, Gagie and
// Navarro (TALG 2025, Section 4.1).  Reads PREFIX.lvl and PREFIX.ptxt; writes a report to stdout.  No timing.
//   bin/parse-samples PREFIX
#include "twolevel.hpp"
#include "tloc_build.hpp"
#include "libsais.h"
#include <cmath>
using namespace katka;

static double mb(u64 b) { return b / 1e6; }

// phi^-1 from (SA at the end of each run -> SA at the start of the next): keys sorted, as twolevel_locate
struct phi_inv_t {
    EliasFano keys; PackedInts vals;
    void build(std::vector<std::pair<u64, u64>> kv, u64 universe) {
        std::sort(kv.begin(), kv.end());
        std::vector<u64> k(kv.size()), v(kv.size());
        for (size_t q = 0; q < kv.size(); ++q) { k[q] = kv[q].first; v[q] = kv[q].second; }
        keys.build(k, universe); vals.build(v);
    }
    u64 operator()(u64 i) const { u64 t = keys.count_less(i + 1) - 1; return vals.get(t) + (i - keys.access(t)); }
    u64 bytes() const { return keys.bytes() + vals.bytes(); }
};

static void gaps(const char *what, std::vector<u64> t, const std::vector<u64> &lim, const std::vector<u64> &svals) {
    std::sort(t.begin(), t.end());
    t.erase(std::unique(t.begin(), t.end()), t.end());
    std::vector<u64> h(lim.size() + 1, 0);
    for (size_t i = 1; i < t.size(); ++i) {
        const u64 g = t[i] - t[i - 1];
        size_t b = 0; while (b < lim.size() && g > lim[b]) ++b;
        ++h[b];
    }
    printf("gaps (%s), %zu samples, %zu gaps:", what, t.size(), t.size() - 1);
    for (size_t b = 0; b <= lim.size(); ++b) {
        const u64 lo = b ? lim[b - 1] + 1 : 1;
        if (b == lim.size()) printf(" >%lu: %lu", lim.back(), h[b]);
        else if (lo == lim[b]) printf(" %lu: %lu", lo, h[b]);
        else printf(" %lu-%lu: %lu", lo, lim[b], h[b]);
        printf(b == lim.size() ? "\n" : ";");
    }
    for (u64 s : svals) {                    // remove t_i if t_{i+1} - t_prev <= s
        u64 keep = std::min<u64>(t.size(), 2), prev = t[0];
        for (size_t i = 1; i + 1 < t.size(); ++i) {
            if (t[i + 1] - prev <= s) continue;
            ++keep; prev = t[i];
        }
        printf("subsampled (%s), s = %lu: %lu remain (%.3f)\n", what, s, keep, (double)keep / t.size());
    }
}

int main(int argc, char **argv) {
    if (argc != 2) die("usage: parse-samples PREFIX");
    const std::string prefix = argv[1];
    double t0 = now();
    two_level L; L.load(prefix + ".lvl");
    parse_text PT;
    if (!parse_text::current_format(prefix + ".ptxt")) die(prefix + ".ptxt is of the old format");
    PT.load(prefix + ".ptxt");
    if (PT.m != L.m || PT.n != L.n || (int32_t)PT.K != L.K) die("the parse does not match the two-level index");
    const u64 n = L.n, m = L.m, N = m + 1;
    if (FILE *f = fopen((prefix + ".tloc").c_str(), "rb")) {     // the parts of PREFIX.tloc as built
        fclose(f);
        twolevel_locate TL; TL.load(prefix + ".tloc");
        printf("%s.tloc as built: characters: run samples %.3f + phi^-1 %.3f MB; parse: run samples %.3f + phrase -> text %.3f MB; "
               "blocks: run samples %.3f + block -> phrase %.3f MB\n", prefix.c_str(), mb(TL.csamp.bytes()), mb(TL.ikeys.bytes() + TL.ivals.bytes()),
               mb(TL.prow.bytes() + TL.psamp.bytes()), mb(TL.pstarts.bytes()), mb(TL.brow.bytes() + TL.bsamp.bytes()), mb(TL.bstarts.bytes()));
    }
    printf("%s: n = %lu, phrases m = %lu, distinct phrases sigma = %u, K = %u, mean distance between phrase starts n/m = %.2f\n",
           prefix.c_str(), n, m, PT.sigma, PT.K, (double)n / m);
    printf("parse BWT runs r_Z = %lu (n_Z = m + 1 = %lu), blocks %lu, block BWT runs r_B = %lu\n", L.pbwt.nruns, N, L.nblocks, L.bbwt.nruns);
    // overlap: the length of phrase i minus the distance to the next phrase start
    {
        u64 mn = UINT64_MAX, mx = 0;
        for (u64 i = 0; i + 1 < m; ++i) {
            const int64_t d = (int64_t)PT.len_of(PT.zid(i)) - (int64_t)(PT.start(i + 1) - PT.start(i));
            mn = std::min<u64>(mn, (u64)d); mx = std::max<u64>(mx, (u64)d);
        }
        printf("overlap (phrase length minus the distance to the next start): min %lu, max %lu\n", mn, mx);
    }
    std::vector<uint32_t> Z(N);
    for (u64 i = 0; i < m; ++i) Z[i] = (uint32_t)PT.zid(i);
    Z[m] = 0;
    PT.ids = PackedInts();                    // (the ids are in Z now; the starts stay)
    std::vector<uint32_t> SA(N);
    if (libsais_int(reinterpret_cast<int32_t *>(Z.data()), reinterpret_cast<int32_t *>(SA.data()), (int32_t)N, (int32_t)L.sigma + 1, 0) != 0) die("libsais_int failed");
    std::vector<uint32_t>().swap(Z);
    fprintf(stderr, "suffix array of the parse: %.1f s\n", now() - t0);
    auto T = [&](u64 v) -> u64 { return v >= m ? n : PT.start(v); };   // phrase index -> text position (m: the end)
    // the run starts of the parse BWT, in row order
    std::vector<u64> rows;
    {
        const EFRank &E = L.pbwt;
        if (E.n != N) die("the parse BWT does not match the parse");
        rows.resize(E.nruns);
        for (u64 j = 0; j < E.nruns; ++j) rows[j] = E.KS.access(j) % N;
        std::sort(rows.begin(), rows.end());
        if (rows[0] != 0) die("internal error: row 0 does not start a run");
    }
    const u64 R = rows.size();
    std::vector<u64> sp(R), st(R);           // SA_Z at the run starts, as phrase indices and text positions
    for (u64 q = 0; q < R; ++q) { sp[q] = SA[rows[q]]; st[q] = T(SA[rows[q]]); }
    {
        EliasFano prow; prow.build(rows, N);
        PackedInts a, b; a.build(sp); b.build(st);
        printf("parse run samples: rows %.3f MB + SA_Z %.3f MB = %.3f MB (phrase indices); with text positions %.3f + %.3f = %.3f MB\n",
               mb(prow.bytes()), mb(a.bytes()), mb(prow.bytes() + a.bytes()), mb(prow.bytes()), mb(b.bytes()), mb(prow.bytes() + b.bytes()));
    }
    // phi^-1 over the parse: SA_Z at the end of run q -> SA_Z at the start of run q + 1
    phi_inv_t PI, PIt;
    {
        std::vector<std::pair<u64, u64>> kv(R - 1), kt(R - 1);
        for (u64 q = 0; q + 1 < R; ++q) {
            const u64 e = SA[rows[q + 1] - 1], s = SA[rows[q + 1]];
            kv[q] = {e, s}; kt[q] = {T(e), T(s)};
        }
        PI.build(std::move(kv), N); PIt.build(std::move(kt), n + 1);
    }
    printf("phi^-1 over the parse: %.3f MB (keys %.3f + values %.3f) in phrase indices; %.3f MB (%.3f + %.3f) in text positions\n",
           mb(PI.bytes()), mb(PI.keys.bytes()), mb(PI.vals.bytes()), mb(PIt.bytes()), mb(PIt.keys.bytes()), mb(PIt.vals.bytes()));
    // (the same Elias-Fano as twolevel_locate's pstarts: built from the same starts with universe n)
    printf("phrase -> text (Elias-Fano over the phrase starts): %.3f MB\n", mb(PT.starts.bytes()));
    // the blocks: their first phrases, and the block BWT's run samples, as block indices and as text positions
    {
        std::vector<u64> bs;
        for (u64 r = 1; r <= L.nblocks; ++r) bs.push_back(SA[L.BS.access(r)]);
        std::sort(bs.begin(), bs.end());
        EliasFano bstarts; bstarts.build(bs, m);
        const EFRank &E = L.bbwt;
        const u64 NB = L.nblocks + 1;
        std::vector<std::pair<u64, u64>> s(E.nruns);
        for (u64 j = 0; j < E.nruns; ++j) { const u64 x = E.KS.access(j) % NB; s[j] = {x, SA[L.BS.access(x)]}; }
        std::sort(s.begin(), s.end());
        std::vector<u64> br(s.size()), bi(s.size()), bt(s.size());
        for (size_t q = 0; q < s.size(); ++q) {
            br[q] = s[q].first; bt[q] = T(s[q].second);
            const u64 v = s[q].second;
            bi[q] = v >= m ? L.nblocks : (u64)(std::lower_bound(bs.begin(), bs.end(), v) - bs.begin());
        }
        EliasFano brow; brow.build(br, NB);
        PackedInts a, b; a.build(bi); b.build(bt);
        printf("block run samples: rows %.3f + block indices %.3f = %.3f MB; with text positions %.3f + %.3f = %.3f MB; block -> phrase %.3f MB\n",
               mb(brow.bytes()), mb(a.bytes()), mb(brow.bytes() + a.bytes()), mb(brow.bytes()), mb(b.bytes()), mb(brow.bytes() + b.bytes()), mb(bstarts.bytes()));
    }
    // check: phi^-1 in phrase indices and in text positions, at every row but the last
    {
        u64 bad = 0, badt = 0;
        for (u64 x = 0; x + 1 < N; ++x) {
            if (PI(SA[x]) != SA[x + 1]) ++bad;
            if (PIt(T(SA[x])) != T(SA[x + 1])) ++badt;
        }
        printf("check of phi^-1 over all %lu rows but the last: %lu wrong in phrase indices, %lu wrong in text positions\n", N - 1, bad, badt);
    }
    fprintf(stderr, "checks: %.1f s\n", now() - t0);
    // regular samples: SA_Z at every row whose phrase index is a multiple of s (and
    // the row of m), as text positions, the rows marked in an Elias-Fano; an occurrence walks forward with Psi to a
    // sampled row, adding the distance between phrase starts (the phrase's length minus K) at each step
    for (u64 s : {16, 32, 64, 128}) {
        std::vector<u64> rw, tp;
        for (u64 x = 0; x < N; ++x) if (SA[x] % s == 0 || SA[x] == m) rw.push_back(x);
        for (u64 x : rw) tp.push_back(T(SA[x]));
        EliasFano mark; mark.build(rw, N);
        PackedInts v; v.build(tp);
        printf("regular samples, s = %lu: %zu samples, rows %.3f + text positions %.3f = %.3f MB\n", s, rw.size(), mb(mark.bytes()), mb(v.bytes()),
               mb(mark.bytes() + v.bytes()));
    }
    {   // the parse BWT's run heads in row order (to read the phrase preceding a row): rows as prow, heads packed
        const EFRank &E = L.pbwt;
        std::vector<u64> hy(R);
        std::vector<std::pair<u64, u64>> h(R);
        for (u64 j = 0; j < R; ++j) { const u64 k = E.KS.access(j); h[j] = {k % N, k / N}; }
        std::sort(h.begin(), h.end());
        for (u64 j = 0; j < R; ++j) hy[j] = h[j].second;
        PackedInts a; a.build(hy);
        printf("run heads of the parse BWT in row order: %.3f MB (+ the rows, as the run samples' %s)\n", mb(a.bytes()), "rows");
    }
    // Psi over the parse BWT (the EFRank: runs grouped by phrase, FP their positions in F, increasing):
    // Psi(i) = start of the run j holding F-row i + (i - FP[j]); the phrase F[i] = KS[j] / N
    {
        const EFRank &E = L.pbwt;
        if (!E.slot.empty()) die("the parse BWT has slots");
        auto psi = [&](u64 i, u64 &c) -> u64 {
            const u64 j = E.FP.count_less(i + 1) - 1, k = E.KS.access(j);
            c = k / N;
            return k % N + (i - E.FP.access(j));
        };
        u64 bad = 0;                                         // check at every row but the one of the suffix $
        for (u64 x = 0; x < N; ++x) {
            if (SA[x] == m) continue;
            u64 c; const u64 y = psi(x, c);
            if (SA[y] != SA[x] + 1 || T(SA[x]) + PT.len_of(c) - PT.K != T(SA[y])) {
                if (!bad) printf("Psi: first wrong row %lu: phrase %u of %lu, Psi row's phrase %u, phrase length %lu, next start %lu - start %lu = %lu\n",
                                 x, SA[x], m, SA[y], PT.len_of(c), T(SA[y]), T(SA[x]), T(SA[y]) - T(SA[x]));
                ++bad;
            }
        }
        printf("check of Psi and the phrase it passes, over all %lu rows but that of $: %lu wrong\n", N - 1, bad);
        // timing: chains of 64 dependent Psi steps from random rows (fixed seed), adding the distance to the next start
        std::vector<u64> from(200000);
        uint64_t z = 0x2545f4914f6cdd1dull;
        for (auto &x : from) { z ^= z << 13; z ^= z >> 7; z ^= z << 17; x = z % N; }
        for (int rep = 0; rep < 3; ++rep) {
            u64 sum = 0, steps = 0;
            const double t1 = now();
            for (u64 x0 : from) {
                u64 x = x0;
                for (int k = 0; k < 64; ++k) {
                    u64 c; const u64 y = psi(x, c);
                    if (c == 0) break;                       // the suffix $ (its F symbol is 0)
                    sum += PT.len_of(c) - PT.K; x = y; ++steps;
                }
            }
            const double t2 = now();
            printf("Psi timing, pass %d: %lu steps in %.3f s, %.1f ns per step (with the phrase length) [%lu]\n", rep + 1, steps, t2 - t1,
                   (t2 - t1) * 1e9 / steps, sum % 7);
        }
    }
    std::vector<uint32_t>().swap(SA);
    // item 5: the gaps between the parse run samples, and the subsampling
    gaps("phrase indices", sp, {1, 2, 3, 4, 8, 16, 32, 64}, {2, 4, 8, 16, 32, 64});
    const u64 d = (u64)std::llround((double)n / m);
    std::vector<u64> lc, sc;
    for (u64 x : {1, 2, 3, 4, 8, 16, 32, 64}) lc.push_back(x * d);
    for (u64 x : {2, 4, 8, 16, 32, 64}) sc.push_back(x * d);
    printf("in characters, the bounds and s are those in phrases times %lu (n/m rounded)\n", d);
    gaps("text positions", st, lc, sc);
    fprintf(stderr, "done: %.1f s\n", now() - t0);
}
