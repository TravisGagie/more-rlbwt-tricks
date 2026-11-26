// Building shortcuts over phrases (pshortcut.hpp) from a two-level index alone: the parse and its suffix array
// are recovered by inverting the parse BWT, so neither the text nor its BWT is read.
//
//   ref:T            the reference is the first sequence (S = G_1 X rc(G_1) X ...: the parse up to the first
//                    phrase that is not over A/C/G/T); every T-th phrase w = P[q] of it (q = T, 2T, ...) gets the
//                    stretch alpha = P[q - T .. q) (the first stretch for w if w recurs)
//   heavy:theta[,kmax[,minocc]]
//                    from each phrase w occurring at least minocc times (default 2), follow the heaviest edge of the
//                    graph with an edge v -> u weighted by the number of times u precedes v (u_1 = the most frequent
//                    predecessor of w, u_2 that of u_1, ...) while at least theta of w's occurrences are preceded by
//                    u_t .. u_1 and t <= kmax (default 64); alpha = u_k .. u_1 for the last such k, if k >= 2
#pragma once
#include "pshortcut.hpp"
#include "twolevel_build.hpp"

namespace katka {

// The parse P[0..m] (ids, P[m] = 0), its suffix array SA[0..m] and C (the first row of each id), by inverting
// the parse BWT: the LF array is overwritten by SA as the inversion visits each row once.
inline void parse_sa_from_pbwt(const two_level &L, std::vector<uint32_t> &P, std::vector<uint32_t> &SA, std::vector<uint64_t> &C) {
    const uint64_t m = L.m, N = m + 1;
    const EFRank &W = L.pbwt;
    if (W.n != N || !W.slot.empty()) die("two-level: unexpected parse BWT layout");
    std::vector<uint32_t> &LF = SA;
    LF.assign(N, 0); P.assign(N, 0);
    C.assign((uint64_t)L.sigma + 2, N);
    BitRank Fb; Fb.init(N);
    uint64_t prevc = UINT64_MAX, nchars = 0;
    for (uint64_t j = 0; j < W.nruns; ++j) {
        uint64_t ks = W.KS.access(j), c = ks / N, start = ks % N, f = W.FP.access(j), fn = j + 1 < W.nruns ? W.FP.access(j + 1) : N;
        if (c != prevc) { Fb.set(f); C[c] = f; prevc = c; ++nchars; }
        for (uint64_t t = 0; t < fn - f; ++t) LF[start + t] = (uint32_t)(f + t);
    }
    Fb.finish();
    if (nchars != (uint64_t)L.sigma + 1) die("two-level: not every phrase id occurs in the parse BWT");
    uint64_t r = 0;                                      // row 0: the suffix at m
    for (uint64_t k = m; k > 0; --k) { uint32_t x = LF[r]; P[k - 1] = x; LF[r] = (uint32_t)k; r = x; }
    if (Fb.rank1((uint64_t)LF[r] + 1) - 1 != 0) die("two-level: the parse BWT's inversion did not end at the terminator");
    LF[r] = 0;                                           // the row of position 0
    for (uint64_t k = 0; k < m; ++k) P[k] = (uint32_t)(Fb.rank1((uint64_t)P[k] + 1) - 1);
    P[m] = 0;
}

inline void build_pshortcuts(const two_level &L, const std::string &spec, parse_shortcuts &SC) {
    double t0 = now();
    bool heavy = false; uint64_t T = 0, kmax = 64, minocc = 2; double theta = 0.9;
    if (spec.rfind("ref:", 0) == 0) T = std::stoull(spec.substr(4));
    else if (spec.rfind("heavy:", 0) == 0) {
        heavy = true;
        if (sscanf(spec.c_str() + 6, "%lf,%lu,%lu", &theta, &kmax, &minocc) < 1) die("--pshortcuts heavy:theta[,kmax[,minocc]]");
    } else die("--pshortcuts ref:T or heavy:theta[,kmax[,minocc]]");
    if (!heavy) { if (T < 2) die("ref:T needs T >= 2"); kmax = T; }
    if (kmax < 2) die("kmax must be at least 2");
    const uint64_t m = L.m;
    const uint32_t sigma = L.sigma;
    std::vector<uint32_t> P, SA; std::vector<uint64_t> C;
    parse_sa_from_pbwt(L, P, SA, C);
    fprintf(stderr, "parse and its suffix array recovered (%.1f s)\n", now() - t0);
    auto occ = [&](uint32_t w) { return C[w + 1] - C[w]; };

    // the candidate stretches: alpha of w as u_1 .. u_k (u_1 next to w), per w (empty: none)
    std::vector<std::vector<uint32_t>> stretch;          // only for ref; heavy computes them on the fly
    std::vector<uint32_t> hp;                            // heavy: the most frequent predecessor of each id
    uint64_t q_end = 0;
    if (!heavy) {
        std::vector<uint8_t> acgt(sigma + 1, 0);
        for (uint64_t t = 0; t < L.dict.size; ++t) acgt[L.dict.idf_at(t)] = 1;
        while (q_end < m && acgt[P[q_end]]) ++q_end;
        fprintf(stderr, "reference: phrases 0 .. %lu (the first not over A/C/G/T)\n", q_end);
        stretch.assign(sigma + 1, {});
        for (uint64_t q = T; q < q_end; q += T) {
            uint32_t w = P[q];
            if (!stretch[w].empty()) continue;
            for (uint64_t t = 1; t <= T; ++t) stretch[w].push_back(P[q - t]);
        }
    } else {
        hp.assign(sigma + 1, 0);
        std::vector<uint32_t> buf;
        for (uint32_t v = 1; v <= sigma; ++v) {
            buf.clear();
            for (uint64_t r = C[v]; r < C[v + 1]; ++r) if (SA[r] > 0) buf.push_back(P[SA[r] - 1]);
            if (buf.empty()) continue;
            std::sort(buf.begin(), buf.end());
            uint64_t best = 0;
            for (size_t a = 0; a < buf.size();) {
                size_t b = a + 1;
                while (b < buf.size() && buf[b] == buf[a]) ++b;
                if (b - a > best) { best = b - a; hp[v] = buf[a]; }
                a = b;
            }
        }
        fprintf(stderr, "heaviest predecessors (%.1f s)\n", now() - t0);
    }

    std::vector<uint64_t> Kv, Av, Cv, Bv, ids, exc, fl;
    uint64_t surv_total = 0, occ_total = 0, crossed = 0;
    std::vector<uint32_t> alive, path;
    std::vector<uint8_t> sv;
    for (uint32_t w = 1; w <= sigma; ++w) {
        const uint64_t o = occ(w);
        path.clear(); alive.clear();
        for (uint64_t r = C[w]; r < C[w + 1]; ++r) alive.push_back((uint32_t)r);
        if (heavy) {
            if (o < minocc) continue;
            uint32_t u = w;
            std::vector<uint32_t> next;
            for (uint64_t t = 1; t <= kmax; ++t) {
                u = hp[u];
                if (!u) break;
                next.clear();
                for (uint32_t r : alive) { uint64_t p = SA[r]; if (p >= t && P[p - t] == u) next.push_back(r); }
                if ((double)next.size() < theta * (double)o) break;
                path.push_back(u); alive.swap(next);
            }
        } else {
            if (stretch[w].empty()) continue;
            path = stretch[w];
            std::vector<uint32_t> next;
            for (uint32_t r : alive) {
                uint64_t p = SA[r]; bool ok = p >= path.size();
                for (size_t t = 0; ok && t < path.size(); ++t) ok = P[p - 1 - t] == path[t];
                if (ok) next.push_back(r);
            }
            alive.swap(next);
        }
        if (path.size() < 2 || alive.empty()) continue;
        const uint64_t k = path.size(), ns = alive.size();
        // A: backward search of alpha (u_1 first) from w's rows
        uint64_t lo = C[w], hi = C[w + 1];
        for (uint64_t t = 0; t < k; ++t) { uint64_t o1, o2; L.pbwt.lf2(path[t], lo, hi, o1, o2); lo = o1; hi = o2; }
        if (hi - lo != ns) die("internal error: shortcut interval size");
        // fingerprint of alpha in text order: u_k .. u_1
        uint64_t h = 0;
        for (uint64_t t = k; t-- > 0;) h = parse_shortcuts::addmod(parse_shortcuts::mulmod(h, SC.kr), path[t]);
        const bool survivors_are_exc = ns <= o - ns;
        Bv.push_back(exc.size());
        if (survivors_are_exc) for (uint32_t r : alive) exc.push_back(r);
        else {
            sv.assign(o, 0);
            for (uint32_t r : alive) sv[r - C[w]] = 1;
            for (uint64_t x = 0; x < o; ++x) if (!sv[x]) exc.push_back(C[w] + x);
        }
        fl.push_back(survivors_are_exc);
        ids.push_back(w); Kv.push_back(k); Av.push_back(lo); Cv.push_back(C[w]);
        SC.H.push_back(h);
        surv_total += ns; occ_total += o; crossed += ns * k;
    }
    std::vector<uint32_t>().swap(P); std::vector<uint32_t>().swap(SA);
    SC.spec = spec; SC.m = m; SC.sigma = sigma; SC.kmax = (uint32_t)kmax; SC.nsc = ids.size();
    SC.has.init(sigma + 1);
    for (auto w : ids) SC.has.set(w);
    SC.has.finish();
    SC.K.build(Kv); SC.A.build(Av); SC.Cw.build(Cv); SC.base.build(Bv);
    SC.flag.assign(SC.nsc / 64 + 1, 0);
    for (uint64_t q = 0; q < SC.nsc; ++q) if (fl[q]) SC.flag[q >> 6] |= 1ull << (q & 63);
    SC.X.build(exc, m + 2);
    SC.prepare();
    uint64_t ksum = 0; for (auto x : Kv) ksum += x;
    fprintf(stderr, "shortcuts over phrases (%s): %lu shortcuts (of %u phrases), %.2f phrases each; their entrances' occurrences %lu, "
                    "%.4f preceded by their stretch; %lu exception rows; %.3f MB (%.1f s)\n",
            spec.c_str(), SC.nsc, sigma, SC.nsc ? (double)ksum / SC.nsc : 0.0, occ_total, occ_total ? (double)surv_total / occ_total : 0.0,
            exc.size(), SC.bytes() / 1e6, now() - t0);
    fprintf(stderr, "shortcuts, MB: entrances %.3f, k %.3f, A %.3f, C_w %.3f, base %.3f, fingerprints %.3f, flags %.3f, exceptions %.3f\n",
            SC.has.bytes() / 1e6, SC.K.bytes() / 1e6, SC.A.bytes() / 1e6, SC.Cw.bytes() / 1e6, SC.base.bytes() / 1e6, SC.H.size() * 8 / 1e6,
            SC.flag.size() * 8 / 1e6, SC.X.bytes() / 1e6);
    (void)crossed;
}

}  // namespace katka
