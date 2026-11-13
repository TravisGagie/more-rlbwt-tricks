// Almost-funnels on the text, over one index of both strands (2026-11-12, "almost-funnels on the text, both
// strands"): Reportable Shortcuts' shortcuts (rlbwt-tricks/shortcuts/rs_build/reportable-shortcuts, include/shortcuts.hpp,
// src/build.cpp), built on the move structure of the BWT of S$, which holds every sequence and its reverse complement,
// with almost-funnels along both CHM13 (R, the first sequence of S) and rc(R).  As there:
//   ell        the window length;  max_tier tau: tier-t strings have ell * 2^t characters, 0 <= t <= tau;
//   anchor p   in R: a multiple of ell, ell <= p, p + ell <= |R|;  window beta_p = R[p .. p + ell);
//              tier-t string alpha_{p,t} = R[p - ell 2^t .. p), offered if ell 2^t divides p;
//              in rc(R): p with |R| - p a multiple of ell, so its boundary is that of R's anchor |R| - p (mirrored, as
//              Reportable Shortcuts aligns its reversed index's anchors), and tier t is offered if ell 2^t divides |R| - p
//              (and ell 2^t <= p);
//   entrance   I(beta_p); windows (of both references) with equal intervals are discarded, so entrances are disjoint;
//   sym(i)     for a row i of an entrance, the number of tiers t with the row's suffix preceded by alpha_{p,t} (the row
//              "survives" tier t if sym(i) > t);  f_{p,t} the first row of I(alpha_{p,t} beta_p).
// A search at X = P[j..end) with |X| >= ell whose interval lies in an entrance compares P[j-1], P[j-2], ... with
// alpha_{p,top}, right to left, and takes the largest tier t it matches whose new interval is not empty:
//   I(alpha X) = [f_{p,t} + survivors before lo, f_{p,t} + survivors through hi)  (Lemma 3)
// which is exactly backward search's interval.  The two references are one virtual sequence V = R rc(R) of 2|R|
// characters, R stored once (2 bits per character); anchors are positions in V.
// File: KTTSC001.
#pragma once
#include "move.hpp"
#include "bits.hpp"

namespace katka {

// count of values > t in [0, i) of a wavelet matrix
inline uint64_t wm_count_greater(const WaveletMatrix &W, uint32_t t, uint64_t i) {
    uint64_t s = 0, e = i, g = 0;
    for (uint32_t lv = 0; lv < W.levels; ++lv) {
        const BitRank &b = W.bv[lv];
        const uint64_t s1 = b.rank1(s), e1 = b.rank1(e);
        if ((t >> (W.levels - 1 - lv)) & 1) { s = W.zeros[lv] + s1; e = W.zeros[lv] + e1; }
        else { g += e1 - s1; s -= s1; e -= e1; }
    }
    return g;
}

struct text_shortcuts {
    u64 ell = 0, max_tier = 0, N = 0, n = 0;     // N = |R|, n = rows of the BWT
    PackedInts ref;                              // R, 2 bits per character (A C G T = 0 1 2 3)
    EliasFano starts;                            // the entrances' first rows
    PackedInts anchor, top, sym_start, tgt_start, targets;
    WaveletMatrix sym;
    std::vector<uint64_t> runbit;                // over the move structure's runs: the run holds a row of some entrance

    // statistics of the build
    u64 st_anchors = 0, st_unusable = 0, st_colliding = 0, st_steps = 0, st_deaths = 0;

    static inline unsigned char base(int a) { return "ACGT"[a]; }
    inline int vchar(u64 v) const {              // V[v], as 0..3
        return v < N ? (int)ref.get(v) : 3 - (int)ref.get(2 * N - 1 - v);
    }
    inline bool maybe(u64 run) const { return (runbit[run >> 6] >> (run & 63)) & 1; }
    inline u64 rows(u64 e) const { return sym_start.get(e + 1) - sym_start.get(e); }
    // the entrance containing rows [lo, hi], or -1; its first row in `first`
    inline int64_t find(u64 lo, u64 hi, u64 &first) const {
        const u64 k = starts.count_less(lo + 1);
        if (k == 0) return -1;
        first = starts.access(k - 1);
        if (hi >= first + rows(k - 1)) return -1;
        return (int64_t)(k - 1);
    }
    // Lemma 3: [lo, hi] in entrance e (first row `first`) -> [nlo, nhi] for alpha_{p,t} X; false if empty
    inline bool jump(u64 e, u64 first, u64 t, u64 lo, u64 hi, u64 &nlo, u64 &nhi) const {
        const u64 b = sym_start.get(e);
        const u64 before = wm_count_greater(sym, (uint32_t)t, b);
        const u64 a = wm_count_greater(sym, (uint32_t)t, b + (lo - first)) - before;
        const u64 z = wm_count_greater(sym, (uint32_t)t, b + (hi + 1 - first)) - before;
        if (z == a) return false;
        const u64 f = targets.get(tgt_start.get(e) + t);
        nlo = f + a; nhi = f + z - 1;
        return true;
    }
    u64 bytes() const {
        return ref.bytes() + starts.bytes() + anchor.bytes() + top.bytes() + sym_start.bytes() + tgt_start.bytes() + targets.bytes() + sym.bytes() + runbit.size() * 8 + 64;
    }
    void save(const std::string &f) const {
        FILE *o = open_or_die(f, "wb"); wmagic(o, "KTTSC001");
        wr(o, ell); wr(o, max_tier); wr(o, N); wr(o, n);
        ref.save(o); starts.save(o); anchor.save(o); top.save(o); sym_start.save(o); tgt_start.save(o); targets.save(o); sym.save(o); wvec(o, runbit);
        fclose(o);
    }
    void load(const std::string &f) {
        FILE *i = open_or_die(f, "rb"); rmagic(i, "KTTSC001", f);
        rd(i, ell); rd(i, max_tier); rd(i, N); rd(i, n);
        ref.load(i); starts.load(i); anchor.load(i); top.load(i); sym_start.load(i); tgt_start.load(i); targets.load(i); sym.load(i); rvec(i, runbit);
        fclose(i);
    }

    // ---- construction (Reportable Shortcuts, Section 6), with theta = 0 (no pruning) ----
    // R: the reference's characters (any bytes; anchors whose window or tier strings are not all A/C/G/T are lowered or
    // dropped, as in build.cpp)
    void build(const move_index &M, const std::string &R, u64 ell_, u64 tau_) {
        ell = ell_; max_tier = tau_; N = R.size(); n = M.n;
        if (ell == 0 || tau_ > 6) die("text shortcuts: ell > 0 and tau <= 6");
        auto code = [](unsigned char ch) { return ch == 'A' ? 0 : ch == 'C' ? 1 : ch == 'G' ? 2 : ch == 'T' ? 3 : -1; };
        std::vector<int8_t> V(2 * N);            // V = R rc(R), -1 for non-ACGT
        for (u64 i = 0; i < N; ++i) { V[i] = (int8_t)code((unsigned char)R[i]); V[2 * N - 1 - i] = V[i] < 0 ? -1 : (int8_t)(3 - V[i]); }
        {
            std::vector<u64> r2(N); for (u64 i = 0; i < N; ++i) r2[i] = V[i] < 0 ? 0 : (u64)V[i];
            ref.build(r2);
        }
        struct win { u64 v, lo, hi; };           // anchor in V, rows [lo, hi]
        std::vector<win> W;
        auto acgt = [&](u64 a, u64 b) { for (u64 k = a; k < b; ++k) if (V[k] < 0) return false; return true; };
        auto search = [&](u64 v, u64 len, move_index::cursor &c) {   // backward search of V[v - len .. v) from c
            for (u64 k = 0; k < len; ++k) { ++st_steps; if (!M.step(c, base(V[v - 1 - k]))) return false; }
            return true;
        };
        for (int h = 0; h < 2; ++h) {
            const u64 base_v = h ? N : 0;
            const u64 first = h ? ell + ((N - ell) % ell + ell) % ell : ell;   // as search_windows (mirrored for rc(R))
            for (u64 p = first; p + ell <= N; p += ell) {
                ++st_anchors;
                if (!acgt(base_v + p, base_v + p + ell)) { ++st_unusable; continue; }
                move_index::cursor c = M.full();
                if (!search(base_v + p + ell, ell, c)) die("text shortcuts: a window of the reference does not occur in the text");
                u64 lo, ep; M.rows(c, lo, ep);
                W.push_back({base_v + p, lo, ep - 1});
            }
        }
        std::sort(W.begin(), W.end(), [](const win &a, const win &b) { return a.lo != b.lo ? a.lo < b.lo : a.v < b.v; });
        std::vector<u64> e_first, e_anchor, e_top, e_symstart{0}, e_tgtstart{0}, e_tgt;
        std::vector<uint8_t> syms;
        std::vector<int32_t> fen;
        for (size_t i = 0; i < W.size();) {
            size_t j = i + 1;
            while (j < W.size() && W[j].lo == W[i].lo) ++j;
            if (j > i + 1) { st_colliding += j - i; i = j; continue; }
            const win w = W[i]; i = j;
            // tau_p (top_tier_of)
            const bool rc = w.v >= N;
            const u64 p = rc ? w.v - N : w.v, b = rc ? N - p : p;
            int64_t tau = 0;
            while ((u64)tau < max_tier && b % (ell << (tau + 1)) == 0 && (ell << (tau + 1)) <= p) ++tau;
            u64 clean = 0;
            while (clean < (ell << tau) && V[w.v - 1 - clean] >= 0) ++clean;
            while (tau >= 0 && (ell << tau) > clean) --tau;
            if (tau < 0) { ++st_unusable; continue; }
            // extend_anchor: track the deaths of the entrance's rows
            const u64 R0 = w.hi - w.lo + 1, s0 = syms.size();
            syms.resize(s0 + R0, (uint8_t)(tau + 1));
            // Fenwick tree over the remaining rows
            fen.assign(R0 + 1, 0);
            for (u64 x = 1; x <= R0; ++x) { fen[x] += 1; const u64 q = x + (x & -x); if (q <= R0) fen[q] += fen[x]; }
            u64 topb = 1; while (topb * 2 <= R0) topb *= 2;
            auto kth = [&](u64 k) { u64 pos = 0; ++k; for (u64 st = topb; st; st >>= 1) if (pos + st <= R0 && (u64)fen[pos + st] < k) { pos += st; k -= fen[pos]; } return pos; };
            auto rem = [&](u64 row) { for (u64 x = row + 1; x <= R0; x += x & -x) --fen[x]; };
            move_index::cursor c = M.at(w.lo, w.hi + 1);
            std::vector<std::pair<u64, u64>> dying;
            u64 reached = 0;
            for (u64 k = 0; k < (ell << tau); ++k) {
                while ((int64_t)reached <= tau && (ell << reached) <= k) ++reached;
                const int ch = V[w.v - 1 - k];
                const uint32_t s = M.sym_of[base(ch)];
                u64 J0, J1; M.rows(c, J0, J1);        // J = [J0, J1)
                dying.clear();
                for (u64 q = c.rs; q <= c.re; ++q) {
                    if (M.sym(q) == s) continue;
                    const u64 qs = M.starts.access(q);
                    const u64 a = std::max(qs, J0), z = std::min(qs + M.len(q), J1);
                    if (!dying.empty() && dying.back().second + 1 == a - J0) dying.back().second = z - 1 - J0;
                    else dying.emplace_back(a - J0, z - 1 - J0);
                }
                for (auto it = dying.rbegin(); it != dying.rend(); ++it)
                    for (u64 rk = it->second + 1; rk-- > it->first;) { const u64 row = kth(rk); syms[s0 + row] = (uint8_t)reached; rem(row); ++st_deaths; }
                ++st_steps;
                if (!M.step(c, base(ch))) die("text shortcuts: internal error (the reference does not occur)");
                const u64 done = k + 1;
                if (done % ell == 0 && ((done / ell) & (done / ell - 1)) == 0) { u64 a, z; M.rows(c, a, z); e_tgt.push_back(a); }
            }
            e_first.push_back(w.lo); e_anchor.push_back(w.v); e_top.push_back((u64)tau);
            e_symstart.push_back(syms.size()); e_tgtstart.push_back(e_tgt.size());
        }
        auto pack = [](PackedInts &P, const std::vector<u64> &v) { P.build(v); };
        starts.build(e_first, n);
        pack(anchor, e_anchor); pack(top, e_top); pack(sym_start, e_symstart); pack(tgt_start, e_tgtstart); pack(targets, e_tgt);
        sym.build(syms, (uint32_t)max_tier + 2);
        runbit.assign(M.r / 64 + 1, 0);
        for (size_t e = 0; e < e_first.size(); ++e) {
            const u64 a = e_first[e], z = a + (e_symstart[e + 1] - e_symstart[e]) - 1;
            for (u64 q = M.starts.pred_index(a), qe = M.starts.pred_index(z); q <= qe; ++q) runbit[q >> 6] |= 1ull << (q & 63);
        }
    }
    u64 entrances() const { return anchor.size(); }
};

}  // namespace katka
