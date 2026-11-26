// Locating from the parse alone with a regular sample (2026-11-26): the rows of the parse's BWT whose suffix starts at
// a phrase index that is a multiple of s (and the rows of the last phrase and of $), marked in an Elias-Fano array,
// with the text position of each one's phrase start.  An occurrence's row walks forward with Psi over the parse's BWT
// (two_level::pbwt, an EFRank: runs grouped by phrase, FP their positions in F) to a marked row, adding the distance to
// the next phrase start, the phrase's length minus K, at each step: at most s - 1 steps.  The phrase lengths come from
// the text-free index (textless_index[2]::len).  Built by bin/prs-build (experiments/parse_locate/prs_build.cpp).
#pragma once
#include "twolevel.hpp"

namespace katka {

struct parse_regular_sample {
    uint64_t s = 0, N = 0, n = 0, m = 0;   // the sampling step; rows of the parse's BWT (m + 1); the text's length; phrases
    EliasFano mark;                         // the sampled rows
    PackedInts pos;                         // the text position of each sampled row's phrase start (n for $)
    uint64_t bytes_rows() const { return mark.bytes(); }
    uint64_t bytes_pos() const { return pos.bytes(); }
    uint64_t bytes() const { return bytes_rows() + bytes_pos(); }
    void save(const std::string &path) const {
        FILE *f = open_or_die(path, "wb"); wmagic(f, "KTPRS001");
        wr(f, s); wr(f, N); wr(f, n); wr(f, m); mark.save(f); pos.save(f);
        fclose(f);
    }
    void load(const std::string &path) {
        FILE *f = open_or_die(path, "rb"); rmagic(f, "KTPRS001", path);
        rd(f, s); rd(f, N); rd(f, n); rd(f, m); mark.load(f); pos.load(f);
        fclose(f);
    }
    // the text position of the phrase start of row r: Psi to a sampled row (steps: the Psi steps taken)
    template <class TI>
    inline uint64_t locate(const EFRank &W, const TI &T, uint64_t r, uint64_t K, uint64_t &steps) const {
        uint64_t off = 0, k;
        while (!mark.find(r, k)) {
            const uint64_t j = W.FP.count_less(r + 1) - 1, ks = W.KS.access(j);
            off += T.len(ks / N) - K;
            r = ks % N + (r - W.FP.access(j));
            ++steps;
        }
        return pos.get(k) - off;
    }
    // the same, with the predecessor in FP by one select-zero (EliasFano::pred) instead of count_less and access
    template <class TI>
    inline uint64_t locate1(const EFRank &W, const TI &T, uint64_t r, uint64_t K, uint64_t &steps) const {
        uint64_t off = 0, k, j, f;
        while (!mark.find(r, k)) {
            W.FP.pred(r, j, f);
            const uint64_t ks = W.KS.access(j);
            off += T.len(ks / N) - K;
            r = ks % N + (r - f);
            ++steps;
        }
        return pos.get(k) - off;
    }
    // the walks from rows[0..cnt), out[q] for rows[q]: `lanes` walks at a time (at most 64), advanced one Psi step each in
    // turn, in two passes over the lanes: the predecessor in FP, prefetching KS's run j; then KS[j], the next row, the
    // sample test, prefetching the phrase's length, which is added in the next pass.
    template <class TI>
    void locate_many(const EFRank &W, const TI &T, const uint64_t *rows, uint64_t cnt, uint64_t K, uint64_t *out, uint64_t &steps,
                     unsigned lanes) const {
        constexpr unsigned MAXL = 64;
        if (lanes > MAXL) lanes = MAXL;
        if (lanes < 1) lanes = 1;
        uint64_t r[MAXL], off[MAXL], dst[MAXL], pc[MAXL], j[MAXL], f[MAXL], fk[MAXL];
        bool fin[MAXL];
        uint64_t next = 0, st = 0;
        const EliasFano &KS = W.KS;
        // a lane takes the next row that isn't sampled itself (those are answered at once); false: none left
        auto take = [&](unsigned i) -> bool {
            while (next < cnt) {
                const uint64_t x = rows[next], q = next++; uint64_t k;
                if (mark.find(x, k)) { out[q] = pos.get(k); continue; }
                r[i] = x; off[i] = 0; dst[i] = q; pc[i] = 0; fin[i] = false;
                return true;
            }
            return false;
        };
        unsigned live = 0;
        while (live < lanes && take(live)) ++live;
        while (live) {
            for (unsigned i = 0; i < live;) {         // pass 1
                if (pc[i]) { off[i] += T.len(pc[i]) - K; pc[i] = 0; }
                if (fin[i]) {
                    out[dst[i]] = pos.get(fk[i]) - off[i];
                    if (!take(i)) { --live; if (i < live) { r[i] = r[live]; off[i] = off[live]; dst[i] = dst[live]; pc[i] = pc[live]; fin[i] = fin[live]; fk[i] = fk[live]; } continue; }
                }
                W.FP.pred(r[i], j[i], f[i]);
                __builtin_prefetch(&KS.low[(j[i] * KS.l) >> 6]);
                if (!KS.f1.empty()) __builtin_prefetch(&KS.f1[j[i] / EliasFano::SAMPLE1]);
                ++i;
            }
            for (unsigned i = 0; i < live; ++i) {     // pass 2
                const uint64_t ks = KS.access(j[i]), c = ks / N;
                r[i] = ks % N + (r[i] - f[i]);
                pc[i] = c;
                if (T.coff.l) __builtin_prefetch(&T.coff.low[((c - 1) * T.coff.l) >> 6]);
                if (!T.coff.f1.empty()) __builtin_prefetch(&T.coff.f1[(c - 1) / EliasFano::SAMPLE1]);
                fin[i] = mark.find(r[i], fk[i]);
                ++st;
            }
        }
        steps += st;
    }

    // ---- the walks as a pipeline: each lane advances one stage per pass, and each stage reads one level of the Elias-Fano
    // arrays that the previous stage prefetched (select-zero: the samples and block counts, then high's block, then the low
    // parts; select-one: the fine sample, then high's words and the low part).  A Psi step is five stages:
    //   0: FP's and mark's block for r (s0, ones), prefetch their high words; the pending phrase's fine sample -> its high
    //   1: FP's and mark's bucket in high, prefetch their low parts; the pending phrase's length (coff) is added
    //   2: the sample test (mark's low parts); FP's predecessor j and F[j]; prefetch KS's fine sample and low part
    //   3: KS's fine sample, prefetch KS's high words
    //   4: KS[j]: the next row and the pending phrase, prefetch its fine sample in coff
    struct lane_t {
        uint64_t r, off, dst, pc;           // row, distance walked, output slot, pending phrase (0: none)
        uint64_t fz_blk, fz_rem, mz_blk, mz_rem;   // stage 0 -> 1: the blocks of the select-zeros (UINT64_MAX: bucket 0)
        uint64_t flo, fhi, fp, mlo, mhi;    // stage 1 -> 2: the buckets (fp: the zero before FP's bucket)
        uint64_t j, f, p1;                  // stage 2 -> 3 -> 4
        uint64_t cw, ca;                    // the pending phrase's high position in coff (stage 0 -> 1)
    };
    static inline void pf(const void *p) { __builtin_prefetch(p); }
    // the block (of 512 bits) holding the k-th zero of E.high, and the zeros before it in the block
    static inline void zero_block(const EliasFano &E, uint64_t k, uint64_t &blk, uint64_t &rem) {
        const uint64_t i = k / EliasFano::SAMPLE;
        uint64_t lo = E.s0[i] >> 9, hi = (i + 1 < E.s0.size()) ? (E.s0[i + 1] >> 9) + 1 : E.ones.size() - 1;
        while (hi - lo > 1) { const uint64_t mid = (lo + hi) >> 1; if ((mid << 9) - E.ones[mid] <= k) lo = mid; else hi = mid; }
        blk = lo; rem = k - ((lo << 9) - E.ones[lo]);
    }
    static inline uint64_t zero_in_block(const EliasFano &E, uint64_t blk, uint64_t rem) {
        for (uint64_t w = blk * 8;; ++w) {
            const uint64_t word = ~E.high[w], c = __builtin_popcountll(word);
            if (rem < c) return (w << 6) + select_in_word(word, rem);
            rem -= c;
        }
    }
    static inline void pf_block(const EliasFano &E, uint64_t blk) { pf(&E.high[blk * 8]); pf(&E.high[blk * 8 + 7]); }
    static inline void pf_low(const EliasFano &E, uint64_t j) { if (E.l) pf(&E.low[(j * E.l) >> 6]); }
    // stage 0 of a bucket search for x: the select-zero's block for bucket x >> l (blk = UINT64_MAX: bucket 0)
    static inline void bucket0(const EliasFano &E, uint64_t x, uint64_t &blk, uint64_t &rem) {
        const uint64_t h = x >> E.l;
        if (h == 0) { blk = UINT64_MAX; rem = 0; return; }
        zero_block(E, h - 1, blk, rem); pf_block(E, blk);
    }
    // stage 1: the bucket [lo, hi) and p, the zero before it
    static inline void bucket1(const EliasFano &E, uint64_t x, uint64_t blk, uint64_t rem, uint64_t &lo, uint64_t &hi, uint64_t &p) {
        const uint64_t h = x >> E.l;
        if (blk == UINT64_MAX) { lo = 0; p = 0; uint64_t b, r; zero_block(E, 0, b, r); hi = zero_in_block(E, b, r); }
        else { p = zero_in_block(E, blk, rem); lo = p - (h - 1); hi = E.next_zero(p + 1) - h; }
        pf_low(E, lo);
    }
    // the first index in [lo, hi) whose low part is >= xl (low parts sorted within the bucket)
    static inline uint64_t first_ge(const EliasFano &E, uint64_t lo, uint64_t hi, uint64_t xl) { return E.in_bucket(lo, hi, xl); }

    template <class TI>
    void locate_staged(const EFRank &W, const TI &T, const uint64_t *rows, uint64_t cnt, uint64_t K, uint64_t *out, uint64_t &steps,
                       unsigned lanes) const {
        constexpr unsigned MAXL = 64;
        if (lanes > MAXL) lanes = MAXL;
        if (lanes < 1) lanes = 1;
        lane_t Ln[MAXL];
        bool idle[MAXL];
        const EliasFano &FP = W.FP, &KS = W.KS, &M = mark, &CO = T.coff;
        if (KS.f1.empty() || CO.f1.empty()) { locate_many(W, T, rows, cnt, K, out, steps, lanes); return; }
        uint64_t next = 0, st = 0;
        const uint64_t invN = ~0ull / N;          // floor((2^64 - 1) / N): ks * invN >> 64 is ks / N or one less
        auto pf_s0 = [&](const EliasFano &E, uint64_t x) { const uint64_t h = x >> E.l; if (h) pf(&E.s0[(h - 1) / EliasFano::SAMPLE]); };
        auto start = [&](lane_t &a) -> bool {     // a new row; it starts at the next stage 0
            if (next >= cnt) return false;
            a.r = rows[next]; a.dst = next++; a.off = 0; a.pc = 0;
            pf_s0(FP, a.r); pf_s0(M, a.r);
            return true;
        };
        unsigned live = 0;
        while (live < lanes && start(Ln[live])) idle[live++] = false;
        // the lanes move together, one stage per pass; a lane that reaches a sample takes a new row and waits for stage 0
        while (live) {
            for (unsigned i = 0; i < live; ++i) {     // stage 0
                lane_t &a = Ln[i]; idle[i] = false;
                bucket0(FP, a.r, a.fz_blk, a.fz_rem);
                bucket0(M, a.r, a.mz_blk, a.mz_rem);
                if (a.pc) {
                    const uint64_t q = a.pc - 1, s = q / EliasFano::SAMPLE1;
                    a.cw = CO.f1[s]; a.ca = q - s * EliasFano::SAMPLE1;
                    pf(&CO.high[a.cw >> 6]); pf(&CO.high[(a.cw >> 6) + 1]); pf_low(CO, q); pf_low(CO, q + 1);
                }
            }
            for (unsigned i = 0; i < live; ++i) {     // stage 1
                lane_t &a = Ln[i];
                bucket1(FP, a.r, a.fz_blk, a.fz_rem, a.flo, a.fhi, a.fp);
                { uint64_t mp; bucket1(M, a.r, a.mz_blk, a.mz_rem, a.mlo, a.mhi, mp); }
                if (a.pc) {
                    uint64_t w = a.cw >> 6, word = CO.high[w] & (~0ull << (a.cw & 63)), rem = a.ca;
                    for (;;) { const uint64_t c = __builtin_popcountll(word); if (rem < c) break; rem -= c; word = CO.high[++w]; }
                    uint64_t p = (w << 6) + select_in_word(word, rem);
                    const uint64_t q = a.pc - 1, x0 = ((p - q) << CO.l) | CO.get_low(q);
                    p = CO.next_one(p + 1);
                    const uint64_t x1 = ((p - q - 1) << CO.l) | CO.get_low(q + 1);
                    a.off += x1 - x0; a.pc = 0;
                }
            }
            for (unsigned i = 0; i < live;) {         // stage 2
                lane_t &a = Ln[i];
                const uint64_t ml = a.r & ((1ull << M.l) - 1), k = first_ge(M, a.mlo, a.mhi, ml);
                if (k < a.mhi && M.get_low(k) == ml) {   // a sampled row: done
                    out[a.dst] = pos.get(k) - a.off;
                    if (!start(a)) { --live; if (i < live) { a = Ln[live]; idle[i] = idle[live]; } continue; }
                    idle[i] = true; ++i; continue;
                }
                const uint64_t h = a.r >> FP.l, xl = a.r & ((1ull << FP.l) - 1), c = first_ge(FP, a.flo, a.fhi, xl + 1);
                if (c > a.flo) { a.j = c - 1; a.f = (h << FP.l) | FP.get_low(a.j); }
                else {
                    a.j = a.flo - 1;
                    uint64_t w = (a.fp - 1) >> 6, word = FP.high[w] & (~0ull >> (63 - ((a.fp - 1) & 63)));
                    while (!word) word = FP.high[--w];
                    a.f = ((((w << 6) + 63 - __builtin_clzll(word)) - a.j) << FP.l) | FP.get_low(a.j);
                }
                pf(&KS.f1[a.j / EliasFano::SAMPLE1]); pf_low(KS, a.j);
                ++i;
            }
            for (unsigned i = 0; i < live; ++i) {     // stage 3
                if (idle[i]) continue;
                lane_t &a = Ln[i];
                a.p1 = KS.f1[a.j / EliasFano::SAMPLE1];
                pf(&KS.high[a.p1 >> 6]); pf(&KS.high[(a.p1 >> 6) + 1]);
            }
            for (unsigned i = 0; i < live; ++i) {     // stage 4
                if (idle[i]) continue;
                lane_t &a = Ln[i];
                uint64_t rem = a.j % EliasFano::SAMPLE1, w = a.p1 >> 6, word = KS.high[w] & (~0ull << (a.p1 & 63));
                for (;;) { const uint64_t c = __builtin_popcountll(word); if (rem < c) break; rem -= c; word = KS.high[++w]; }
                const uint64_t p = (w << 6) + select_in_word(word, rem);
                const uint64_t ks = ((p - a.j) << KS.l) | KS.get_low(a.j);
                uint64_t c = (uint64_t)(((unsigned __int128)ks * invN) >> 64), rr = ks - c * N;   // ks / N, ks % N
                while (rr >= N) { ++c; rr -= N; }
                a.r = rr + (a.r - a.f);
                a.pc = c; pf(&CO.f1[(c - 1) / EliasFano::SAMPLE1]);
                pf_s0(FP, a.r); pf_s0(M, a.r);
                ++st;
            }
        }
        steps += st;
    }

    // The walks of one MEM's rows (sorted, rows[0..cnt)), a run at a time: rows of the parse's BWT in one run of F (FP) go
    // to consecutive rows by Psi, with the same phrase, so a piece of consecutive rows walks as one until it crosses a
    // run boundary (it splits) or holds a sampled row (that row is done; the piece shrinks to the rows not done).
    // out[q] for rows[q]; steps counts the pieces' steps.
    template <class TI>
    void locate_runs(const EFRank &W, const TI &T, const uint64_t *rows, uint64_t cnt, uint64_t K, uint64_t *out, uint64_t &steps) const {
        struct piece { uint64_t lo, hi, off, t0; };
        static thread_local std::vector<piece> stk;
        static thread_local std::vector<uint8_t> done;
        stk.clear(); done.assign(cnt, 0);
        for (uint64_t q = 0; q < cnt;) {               // the maximal runs of consecutive rows
            uint64_t z = q + 1;
            while (z < cnt && rows[z] == rows[z - 1] + 1) ++z;
            stk.push_back({rows[q], rows[z - 1], 0, q});
            q = z;
        }
        const EliasFano &FP = W.FP, &KS = W.KS;
        while (!stk.empty()) {
            piece P = stk.back(); stk.pop_back();
            for (;;) {
                uint64_t k1, k2;                          // the sampled rows in [lo, hi]
                mark.count_less2(P.lo, P.hi + 1, k1, k2);
                for (uint64_t k = k1; k < k2; ++k) {
                    const uint64_t t = P.t0 + (mark.access(k) - P.lo);
                    if (!done[t]) { out[t] = pos.get(k) - P.off; done[t] = 1; }
                }
                if (k1 < k2) {
                    while (P.lo <= P.hi && done[P.t0]) { ++P.lo; ++P.t0; }
                    while (P.hi >= P.lo && done[P.t0 + (P.hi - P.lo)]) --P.hi;
                    if (P.lo > P.hi) break;
                }
                uint64_t j, f;
                FP.pred(P.lo, j, f);
                const uint64_t fe = j + 1 < W.nruns ? FP.access(j + 1) : N;   // the run is F[f..fe)
                if (P.hi >= fe) { stk.push_back({fe, P.hi, P.off, P.t0 + (fe - P.lo)}); P.hi = fe - 1; }
                const uint64_t ks = KS.access(j), c = ks / N, nl = ks % N + (P.lo - f);
                P.hi = nl + (P.hi - P.lo); P.lo = nl;
                P.off += T.len(c) - K;
                ++steps;
            }
        }
    }

    // locate_runs's pieces through locate_staged's pipeline: `lanes` pieces at a time.  groups: each MEM's rows, (start,
    // count) in rows[0..cnt) (sorted within a MEM); a piece never mixes MEMs.  A piece's step is the five stages of a row's,
    // with the sampled rows of [lo, hi] (mark's bucket for lo; past it, mark.count_less) and the end of lo's run in F
    // (the element after the predecessor) found at stage 2, where a piece that crosses the end splits (its rest waits on
    // a stack for a free lane).
    template <class TI>
    void locate_staged_runs(const EFRank &W, const TI &T, const uint64_t *rows, uint64_t cnt, const std::pair<uint64_t, uint64_t> *groups,
                            uint64_t ng, uint64_t K, uint64_t *out, uint64_t &steps, unsigned lanes) const {
        constexpr unsigned MAXL = 64;
        if (lanes > MAXL) lanes = MAXL;
        if (lanes < 1) lanes = 1;
        struct piece { uint64_t lo, hi, off, t0; };
        struct pl { piece P; uint64_t pc, fz_blk, fz_rem, mz_blk, mz_rem, flo, fhi, fp, mlo, mhi, j, f, p1, cw, ca; };
        pl Ln[MAXL];
        bool idle[MAXL];
        const EliasFano &FP = W.FP, &KS = W.KS, &M = mark, &CO = T.coff;
        if (KS.f1.empty() || CO.f1.empty()) die("locate_staged_runs: no fine samples");
        static thread_local std::vector<piece> stk;
        static thread_local std::vector<uint8_t> done;
        stk.clear(); done.assign(cnt, 0);
        uint64_t g = 0, gq = 0, st = 0;                // the next MEM, and the next row in it, for a new piece
        const uint64_t invN = ~0ull / N;
        auto pf_s0 = [&](const EliasFano &E, uint64_t x) { const uint64_t h = x >> E.l; if (h) pf(&E.s0[(h - 1) / EliasFano::SAMPLE]); };
        auto take = [&](pl &a) -> bool {
            if (!stk.empty()) { a.P = stk.back(); stk.pop_back(); }
            else {
                while (g < ng && gq >= groups[g].second) { ++g; gq = 0; }
                if (g >= ng) return false;
                const uint64_t b = groups[g].first, c = groups[g].second;
                uint64_t z = gq + 1;
                while (z < c && rows[b + z] == rows[b + z - 1] + 1) ++z;
                a.P = {rows[b + gq], rows[b + z - 1], 0, b + gq};
                gq = z;
            }
            a.pc = 0; pf_s0(FP, a.P.lo); pf_s0(M, a.P.lo);
            return true;
        };
        unsigned live = 0;
        while (live < lanes && take(Ln[live])) idle[live++] = false;
        while (live) {
            for (unsigned i = 0; i < live; ++i) {     // stage 0
                pl &a = Ln[i]; idle[i] = false;
                bucket0(FP, a.P.lo, a.fz_blk, a.fz_rem);
                bucket0(M, a.P.lo, a.mz_blk, a.mz_rem);
                if (a.pc) {
                    const uint64_t q = a.pc - 1, s = q / EliasFano::SAMPLE1;
                    a.cw = CO.f1[s]; a.ca = q - s * EliasFano::SAMPLE1;
                    pf(&CO.high[a.cw >> 6]); pf(&CO.high[(a.cw >> 6) + 1]); pf_low(CO, q); pf_low(CO, q + 1);
                }
            }
            for (unsigned i = 0; i < live; ++i) {     // stage 1
                pl &a = Ln[i];
                bucket1(FP, a.P.lo, a.fz_blk, a.fz_rem, a.flo, a.fhi, a.fp);
                { uint64_t mp; bucket1(M, a.P.lo, a.mz_blk, a.mz_rem, a.mlo, a.mhi, mp); }
                if (a.pc) {
                    uint64_t w = a.cw >> 6, word = CO.high[w] & (~0ull << (a.cw & 63)), rem = a.ca;
                    for (;;) { const uint64_t c = __builtin_popcountll(word); if (rem < c) break; rem -= c; word = CO.high[++w]; }
                    uint64_t p = (w << 6) + select_in_word(word, rem);
                    const uint64_t q = a.pc - 1, x0 = ((p - q) << CO.l) | CO.get_low(q);
                    p = CO.next_one(p + 1);
                    const uint64_t x1 = ((p - q - 1) << CO.l) | CO.get_low(q + 1);
                    a.P.off += x1 - x0; a.pc = 0;
                }
            }
            for (unsigned i = 0; i < live;) {         // stage 2
                pl &a = Ln[i];
                piece &P = a.P;
                {                                       // the sampled rows in [lo, hi]
                    const uint64_t mm = (1ull << M.l) - 1, h = P.lo >> M.l;
                    uint64_t k = first_ge(M, a.mlo, a.mhi, P.lo & mm);
                    bool any = false;
                    for (; k < a.mhi; ++k) {
                        const uint64_t x = (h << M.l) | M.get_low(k);
                        if (x > P.hi) break;
                        const uint64_t t = P.t0 + (x - P.lo);
                        if (!done[t]) { out[t] = pos.get(k) - P.off; done[t] = 1; } any = true;
                    }
                    if (k == a.mhi && ((P.hi + 1) >> M.l) > h)   // past lo's bucket
                        for (const uint64_t k2 = M.count_less(P.hi + 1); k < k2; ++k) {
                            const uint64_t t = P.t0 + (M.access(k) - P.lo);
                            if (!done[t]) { out[t] = pos.get(k) - P.off; done[t] = 1; } any = true;
                        }
                    if (any) {
                        while (P.lo <= P.hi && done[P.t0]) { ++P.lo; ++P.t0; }
                        while (P.hi >= P.lo && done[P.t0 + (P.hi - P.lo)]) --P.hi;
                        if (P.lo > P.hi) {
                            if (!take(a)) { --live; if (i < live) { a = Ln[live]; idle[i] = idle[live]; } continue; }
                            idle[i] = true; ++i; continue;
                        }
                    }
                }
                // (lo may have moved: then FP's bucket is recomputed here)
                uint64_t flo = a.flo, fhi = a.fhi, fp = a.fp;
                const uint64_t h = P.lo >> FP.l, xl = P.lo & ((1ull << FP.l) - 1);
                {
                    uint64_t b0, r0; bucket0(FP, P.lo, b0, r0);
                    if (b0 != a.fz_blk || r0 != a.fz_rem) bucket1(FP, P.lo, b0, r0, flo, fhi, fp);
                }
                const uint64_t c = first_ge(FP, flo, fhi, xl + 1);
                uint64_t fe;
                if (c > flo) { a.j = c - 1; a.f = (h << FP.l) | FP.get_low(a.j); }
                else {
                    a.j = flo - 1;
                    uint64_t w = (fp - 1) >> 6, word = FP.high[w] & (~0ull >> (63 - ((fp - 1) & 63)));
                    while (!word) word = FP.high[--w];
                    a.f = ((((w << 6) + 63 - __builtin_clzll(word)) - a.j) << FP.l) | FP.get_low(a.j);
                }
                if (c >= FP.n) fe = N;
                else if (c < fhi) fe = (h << FP.l) | FP.get_low(c);
                else { const uint64_t q = FP.next_one(fhi + h + 1); fe = ((q - c) << FP.l) | FP.get_low(c); }
                if (P.hi >= fe) { stk.push_back({fe, P.hi, P.off, P.t0 + (fe - P.lo)}); P.hi = fe - 1; }
                pf(&KS.f1[a.j / EliasFano::SAMPLE1]); pf_low(KS, a.j);
                ++i;
            }
            for (unsigned i = 0; i < live; ++i) {     // stage 3
                if (idle[i]) continue;
                pl &a = Ln[i];
                a.p1 = KS.f1[a.j / EliasFano::SAMPLE1];
                pf(&KS.high[a.p1 >> 6]); pf(&KS.high[(a.p1 >> 6) + 1]);
            }
            for (unsigned i = 0; i < live; ++i) {     // stage 4
                if (idle[i]) continue;
                pl &a = Ln[i];
                uint64_t rem = a.j % EliasFano::SAMPLE1, w = a.p1 >> 6, word = KS.high[w] & (~0ull << (a.p1 & 63));
                for (;;) { const uint64_t c = __builtin_popcountll(word); if (rem < c) break; rem -= c; word = KS.high[++w]; }
                const uint64_t p = (w << 6) + select_in_word(word, rem);
                const uint64_t ks = ((p - a.j) << KS.l) | KS.get_low(a.j);
                uint64_t c = (uint64_t)(((unsigned __int128)ks * invN) >> 64), rr = ks - c * N;
                while (rr >= N) { ++c; rr -= N; }
                const uint64_t nl = rr + (a.P.lo - a.f);
                a.P.hi = nl + (a.P.hi - a.P.lo); a.P.lo = nl;
                a.pc = c; pf(&CO.f1[(c - 1) / EliasFano::SAMPLE1]);
                pf_s0(FP, a.P.lo); pf_s0(M, a.P.lo);
                ++st;
            }
        }
        steps += st;
    }
};

}  // namespace katka
