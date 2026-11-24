// Two-level search without the index of the text (stage 2): the parse's BWT, the phrase dictionary of the pattern
// parse, and the distinct phrases stored at 2 bits per base.  It gives the longest suffix X[j..end) of X[begin..end)
// that occurs in the text and the number of its occurrences, as long as X[begin..end) has a phrase boundary (a
// closed syncmer X[s..s+K) with begin <= s, s + K <= end); there is no interval in the BWT of the text.
//
//   right end   the tail T = X[s..end) from the last boundary s.  The phrases that start with T's first K characters
//               (the last K of alpha, the phrase before s) are a range of identifiers, since the identifiers are the
//               phrases' lexicographic order and the parse is prefix-free; T occurs exactly at the boundaries where
//               a phrase starting with T starts.  A binary search by string comparison of T[K..) with the phrases
//               after their first K characters gives the phrases [y_lo, y_hi] that start with T, and T's parse range
//               is [C[y_lo], C[y_hi + 1]).  The K-mer's range comes from alpha (lastr[alpha]) if alpha has an
//               identifier, else from a binary search over the ranges' K-mers.  No phrase starts with T: T does not
//               occur, and the search returns s + 1 (correct for BML's window test, not the exact j).
//   middle      parse steps, as in two-level search.
//   left end    at the boundary s_i where the parse steps stop (a step fails, the next pattern phrase has no
//               identifier, or it is cut by begin), the occurrences of X[s_i..end) are the rows [plo, phi] of the
//               parse's BWT.  For each distinct phrase y preceding them (the run heads by row), compare the pattern
//               leftward from s_i with y without its last K characters, capped at begin.  The longest agreement a
//               gives j = s_i - a; the occurrences are the rows whose y agrees that far.  An agreement over all of y
//               (short of the cap) would mean that the pattern contains y there, so the parse step for y would have
//               succeeded: it is counted as a bug (bug_full_agree).
#pragma once
#include "parse_text.hpp"

namespace katka {

struct textless_index {
    uint32_t K = 0;
    uint64_t sigma = 0, R = 0, N = 0;       // phrases; ranges of identifiers (phrases with the same first K-mer); rows of the parse's BWT
    packed_dict dict;                       // the distinct phrases, whole, 2 bits per base (the parse oracle's dictionary)
    PackedInts doff;                        // phrase y is bases doff[y-1] .. doff[y] of dict
    PackedInts rfirst;                      // range r is identifiers rfirst[r] .. rfirst[r+1]-1 (rfirst[R] = sigma + 1)
    PackedInts lastr;                       // lastr[y]: the range of y's last K-mer (R: none, the phrase before $)
    EliasFano hstart;                       // the parse's BWT by row: the start of each run, in row order
    PackedInts hsym;                        //   and its symbol
    uint64_t hruns = 0;

    inline u64 off(u64 y) const { return doff.get(y - 1); }
    inline u64 len(u64 y) const { return doff.get(y) - doff.get(y - 1); }
    inline void chars(u64 y, u64 from, u64 to, uint8_t *out) const { dict.decode(y, off(y), from, to, out); }
    static constexpr bool whole = true;     // whole phrases: the K-mer search over the ranges is possible
    inline void range(u64 r, u64 &a, u64 &b) const { a = rfirst.get(r); b = rfirst.get(r + 1); }
    inline void right(u64 y, u64 from, u64 to, uint8_t *out) const { chars(y, K + from, K + to, out); }   // y[K + from .. K + to)
    inline void left(u64 y, u64 from, u64 to, uint8_t *out) const { chars(y, from, to, out); }           // y[from .. to), to <= |y| - K
    uint64_t bytes_phrases() const { return dict.w.size() * 8 + dict.marked.bytes() + dict.roff.bytes() + dict.rpos.bytes() + dict.rlen.bytes() + dict.dpos.bytes() + doff.bytes(); }
    uint64_t bytes_ranges() const { return rfirst.bytes() + lastr.bytes(); }
    uint64_t bytes_rows() const { return hstart.bytes() + hsym.bytes(); }
    uint64_t bytes() const { return bytes_phrases() + bytes_ranges() + bytes_rows(); }

    void save(const std::string &path) const {
        FILE *f = open_or_die(path, "wb"); wmagic(f, "KTTLX001");
        wr(f, K); wr(f, sigma); wr(f, R); wr(f, N); wr(f, hruns);
        dict.save(f); doff.save(f); rfirst.save(f); lastr.save(f); hstart.save(f); hsym.save(f);
        fclose(f);
    }
    void load(const std::string &path) {
        FILE *f = open_or_die(path, "rb"); rmagic(f, "KTTLX001", path);
        rd(f, K); rd(f, sigma); rd(f, R); rd(f, N); rd(f, hruns);
        dict.load(f); doff.load(f); rfirst.load(f); lastr.load(f); hstart.load(f); hsym.load(f);
        fclose(f);
    }

    // From the two-level index L and the parse oracle's dictionary (dict, doff of PREFIX.ptxt; ids in L's order).
    // Checks: the identifiers are in strictly increasing lexicographic order of the phrases, each first K-mer is one
    // run of identifiers, and every successor of a phrase starts with its last K-mer (one range per phrase).
    void build(const two_level &L, packed_dict &&d, PackedInts &&o, uint32_t k) {
        K = k; sigma = L.sigma; N = L.pbwt.n; dict = std::move(d); doff = std::move(o);
        const EFRank &W = L.pbwt;
        if (W.n != L.m + 1 || !W.slot.empty()) die("textless: unexpected parse BWT layout");
        std::vector<uint8_t> a, b;
        std::vector<uint64_t> rf; std::vector<uint32_t> rid(sigma + 1, 0);
        for (u64 y = 1; y <= sigma; ++y) {
            const u64 ly = len(y);
            if (ly < K) die("textless: a phrase shorter than K");
            b.resize(ly); chars(y, 0, ly, b.data());
            if (y > 1) {
                const u64 l = std::min<u64>(a.size(), ly);
                const int c = memcmp(a.data(), b.data(), l);
                if (c > 0 || (c == 0 && a.size() >= ly)) die("textless: the identifiers are not in the phrases' lexicographic order");
            }
            if (y == 1 || memcmp(a.data(), b.data(), K)) rf.push_back(y);
            rid[y] = (uint32_t)(rf.size() - 1);
            a.swap(b);
        }
        R = rf.size(); rf.push_back(sigma + 1);
        rfirst.build(rf);
        // lastr from the parse's BWT: a run of symbol x at rows starting with phrase z means x is followed by z
        std::vector<uint64_t> C(sigma + 2, 0);
        for (u64 j = 0; j < W.nruns; ++j) {
            const u64 ks = W.KS.access(j), f = W.FP.access(j), fn = j + 1 < W.nruns ? W.FP.access(j + 1) : N;
            C[ks / N + 1] += fn - f;
        }
        for (u64 y = 1; y <= sigma + 1; ++y) C[y] += C[y - 1];
        std::vector<uint64_t> lr(sigma + 1, R);
        std::vector<std::pair<uint64_t, uint32_t>> heads(W.nruns);
        for (u64 j = 0; j < W.nruns; ++j) {
            const u64 ks = W.KS.access(j), x = ks / N, st = ks % N, f = W.FP.access(j), fn = j + 1 < W.nruns ? W.FP.access(j + 1) : N;
            heads[j] = {st, (uint32_t)x};
            const u64 z1 = (u64)(std::upper_bound(C.begin(), C.end(), st) - C.begin() - 1),
                      z2 = (u64)(std::upper_bound(C.begin(), C.end(), st + (fn - f) - 1) - C.begin() - 1);
            if (!x) continue;
            for (u64 z = std::max<u64>(z1, 1); z <= z2; ++z) {
                if (lr[x] == R) lr[x] = rid[z];
                else if (lr[x] != rid[z]) die("textless: the successors of a phrase start with different K-mers");
            }
        }
        lastr.build(lr);
        std::sort(heads.begin(), heads.end());
        hruns = heads.size();
        std::vector<uint64_t> hs(hruns), hy(hruns);
        for (u64 j = 0; j < hruns; ++j) { hs[j] = heads[j].first; hy[j] = heads[j].second; }
        if (hruns && hs[0] != 0) die("textless: the first run does not start at row 0");
        hstart.build(hs, N); hsym.build(hy);
    }
};

// The same at minimal widths (stage 2, part C1): two short copies of each phrase over A/C/G/T, y[K..) (the right end) and
// y[0..|y|-K) (the left end), 2 bits per base, with one Elias-Fano array of offsets for both (they have the same length);
// the phrases with other characters (X, $) whole, as bytes; the ranges as a bit per phrase marking where each starts
// (select gives a range's identifiers) and a pointer per phrase, at ceil(log2(R + 1)) bits, to its last K-mer's range.
// No phrase's first K characters are stored whole (most phrases are shorter than 2K), so there is no K-mer search:
// alpha without an identifier gives its left boundary in window tests, and stops an exact search.
struct textless_index2 {
    static constexpr bool whole = false;
    uint32_t K = 0;
    uint64_t sigma = 0, R = 0, N = 0, hruns = 0;
    std::vector<uint64_t> cr, cl;           // the right and left copies, 32 bases per word
    EliasFano coff;                         // phrase y's copies are bases coff[y-1] .. coff[y] of each
    BitRank marked;                         // phrases with characters other than A/C/G/T
    std::string other; PackedInts ooff;     // their bytes, whole, in identifier order; the r-th is other[ooff[r] .. ooff[r+1])
    BitRank rbits;                          // bit y (1 .. sigma + 1): y starts a range (sigma + 1: the end)
    PackedInts lastr;
    EliasFano hstart; PackedInts hsym;

    inline u64 len(u64 y) const { u64 a, b; coff.access2(y - 1, a, b); return b - a + K; }
    inline void range(u64 r, u64 &a, u64 &b) const { a = rbits.select1(r); b = rbits.select1(r + 1); }
    inline void dec(const std::vector<uint64_t> &w, u64 i, u64 n, uint8_t *out) const {
        static const uint8_t base[4] = {'A', 'C', 'G', 'T'};
        for (u64 t = 0; t < n; ++t, ++i) out[t] = base[(w[i >> 5] >> (2 * (i & 31))) & 3];
    }
    inline const char *whole_of(u64 y) const { const u64 q = marked.rank1(y); return other.data() + ooff.get(q); }
    inline void right(u64 y, u64 from, u64 to, uint8_t *out) const {
        if (marked.get(y)) { memcpy(out, whole_of(y) + K + from, to - from); return; }
        dec(cr, coff.access(y - 1) + from, to - from, out);
    }
    inline void left(u64 y, u64 from, u64 to, uint8_t *out) const {
        if (marked.get(y)) { memcpy(out, whole_of(y) + from, to - from); return; }
        dec(cl, coff.access(y - 1) + from, to - from, out);
    }
    uint64_t bytes_phrases() const { return (cr.size() + cl.size()) * 8 + coff.bytes() + marked.bytes() + other.size() + ooff.bytes(); }
    uint64_t bytes_ranges() const { return rbits.bytes() + lastr.bytes(); }
    uint64_t bytes_rows() const { return hstart.bytes() + hsym.bytes(); }
    uint64_t bytes() const { return bytes_phrases() + bytes_ranges() + bytes_rows(); }
    void save(const std::string &path) const {
        FILE *f = open_or_die(path, "wb"); wmagic(f, "KTTLX002");
        wr(f, K); wr(f, sigma); wr(f, R); wr(f, N); wr(f, hruns);
        wvec(f, cr); wvec(f, cl); coff.save(f); marked.save(f);
        uint64_t os = other.size(); wr(f, os); if (os) fwrite(other.data(), 1, os, f);
        ooff.save(f); rbits.save(f); lastr.save(f); hstart.save(f); hsym.save(f);
        fclose(f);
    }
    void load(const std::string &path) {
        FILE *f = open_or_die(path, "rb"); rmagic(f, "KTTLX002", path);
        rd(f, K); rd(f, sigma); rd(f, R); rd(f, N); rd(f, hruns);
        rvec(f, cr); rvec(f, cl); coff.load(f); marked.load(f);
        uint64_t os = 0; rd(f, os); other.resize(os); if (os && fread(&other[0], 1, os, f) != os) die(path + ": truncated");
        ooff.load(f); rbits.load(f); lastr.load(f); hstart.load(f); hsym.load(f);
        fclose(f);
    }
    // from the fixed-width index (same phrases, ranges, pointers and run heads)
    void build(const textless_index &T) {
        K = T.K; sigma = T.sigma; R = T.R; N = T.N; hruns = T.hruns;
        std::vector<uint64_t> off(sigma + 1, 0), oo{0};
        u64 tot = 0;
        for (u64 y = 1; y <= sigma; ++y) { tot += T.len(y) - K; off[y] = tot; }
        cr.assign(tot / 32 + 2, 0); cl.assign(tot / 32 + 2, 0);
        marked.init(sigma + 1);
        std::vector<uint8_t> b;
        auto put = [&](std::vector<uint64_t> &w, u64 i, uint8_t c) { const int x = c == 'C' ? 1 : c == 'G' ? 2 : c == 'T' ? 3 : 0; w[i >> 5] |= (uint64_t)x << (2 * (i & 31)); };
        for (u64 y = 1; y <= sigma; ++y) {
            const u64 ly = T.len(y); b.resize(ly); T.chars(y, 0, ly, b.data());
            bool acgt = true; for (u64 t = 0; t < ly; ++t) if (b[t] != 'A' && b[t] != 'C' && b[t] != 'G' && b[t] != 'T') { acgt = false; break; }
            if (!acgt) { marked.set(y); other.append((const char *)b.data(), ly); oo.push_back(other.size()); continue; }
            for (u64 t = 0; t + K < ly; ++t) { put(cr, off[y - 1] + t, b[K + t]); put(cl, off[y - 1] + t, b[t]); }
        }
        marked.finish(); ooff.build(oo);
        coff.build(off, tot + 1);
        rbits.init(sigma + 2);
        for (u64 r = 0; r <= R; ++r) rbits.set(T.rfirst.get(r));
        rbits.finish();
        std::vector<uint64_t> lr(sigma + 1); for (u64 y = 0; y <= sigma; ++y) lr[y] = y ? T.lastr.get(y) : R;
        lastr.build(lr);
        hstart = T.hstart; hsym = T.hsym;
    }
};

struct textless_counters {
    u64 searches = 0, no_boundary = 0, window_searches = 0, exact_searches = 0;
    u64 kmer_by_search_exact = 0, alpha_noid_bound = 0, unresolved = 0, kmer_from_alpha = 0, kmer_from_alpha_cut = 0, kmer_by_search_no_id = 0, kmer_by_search_no_alpha = 0, kmer_absent = 0, kmer_alpha_none = 0;
    u64 rj_nosucc = 0, rj_succ = 0, rj_notail = 0, rj_none = 0, rj_cut = 0;   // C2b's cases
    u64 tail_only_kmer = 0, right_cmps = 0, range_size_sum = 0, tail_absent = 0, tail_absent_exact = 0;
    u64 alpha_step_fail = 0;                // the parse step for alpha failed: the tail occurs, not after alpha
    u64 left = 0, left_at_cap = 0, left_distinct_sum = 0, left_distinct_max = 0, left_runs_sum = 0, left_cmps_chars = 0, bug_full_agree = 0;
    u64 left_hist[8] = {};                  // distinct preceding phrases: 0, 1, 2, 3-4, 5-8, 9-64, 65-1024, more
    // with KATKA_TEXTLESS_CHECK (the text index loaded too): every search against two-level search
    u64 fb_forward = 0, fb_forward_bound = 0, fb_back = 0, fb_back_bound = 0, fb_lm_checks = 0, fb_not_lm = 0, fb_stop_short = 0, fb_step1 = 0;   // forward-backward (C4)
    u64 chk = 0, chk_bad_j = 0, chk_bad_occ = 0, chk_window_short = 0, chk_window_lost = 0, chk_window_noocc = 0;
    void hist(u64 d) { left_hist[d == 0 ? 0 : d == 1 ? 1 : d == 2 ? 2 : d <= 4 ? 3 : d <= 8 ? 4 : d <= 64 ? 5 : d <= 1024 ? 6 : 7]++; }
    void print(FILE *f) const {
        fprintf(f, "textless: searches=%lu (window %lu, exact %lu), no boundary %lu\n", searches, window_searches, exact_searches, no_boundary);
        fprintf(f, "textless right end: K-mer range from alpha %lu, from alpha cut by the window's start %lu, by search (alpha has no identifier) %lu, "
                   "by search (no alpha in the pattern) %lu, alpha is the phrase before $ %lu, K-mer absent %lu; tail = the K-mer %lu; "
                   "string comparisons %lu, range sizes %lu; tail absent %lu (in exact searches %lu); alpha's step fails %lu\n",
                kmer_from_alpha, kmer_from_alpha_cut, kmer_by_search_no_id, kmer_by_search_no_alpha, kmer_alpha_none, kmer_absent, tail_only_kmer,
                right_cmps, range_size_sum, tail_absent, tail_absent_exact, alpha_step_fail);
        fprintf(f, "textless left end: %lu (at the window's start %lu); distinct preceding phrases sum %lu, max %lu; runs %lu; characters compared %lu; "
                   "full agreements (bugs) %lu; distinct: 0 %lu, 1 %lu, 2 %lu, 3-4 %lu, 5-8 %lu, 9-64 %lu, 65-1024 %lu, more %lu\n",
                left, left_at_cap, left_distinct_sum, left_distinct_max, left_runs_sum, left_cmps_chars, bug_full_agree,
                left_hist[0], left_hist[1], left_hist[2], left_hist[3], left_hist[4], left_hist[5], left_hist[6], left_hist[7]);
        fprintf(f, "textless: K-mer range needed in exact searches %lu; alpha without identifier, its left boundary reported (window tests) %lu; unresolved (stopped) %lu\n", kmer_by_search_exact, alpha_noid_bound, unresolved);
        if (rj_nosucc + rj_succ + rj_notail + rj_none + rj_cut) fprintf(f, "textless right-end jump: alpha has no successor %lu, its successor starts with the tail %lu, does not %lu, d has no rows %lu, alpha's pair cut by the window %lu\n", rj_nosucc, rj_succ, rj_notail, rj_none, rj_cut);
        if (fb_forward) fprintf(f, "textless forward-backward: forward searches %lu (bounds %lu), backward searches %lu (bounds %lu), left-maximality checks %lu (not left-maximal %lu), stopped with less than L left %lu, restarts by one %lu\n", fb_forward, fb_forward_bound, fb_back, fb_back_bound, fb_lm_checks, fb_not_lm, fb_stop_short, fb_step1);
        if (chk) fprintf(f, "textless check: %lu searches checked; j differs %lu, occurrences differ %lu; window searches with a shorter skip %lu (characters %lu), with the exact j but no count %lu\n",
                         chk, chk_bad_j, chk_bad_occ, chk_window_short, chk_window_lost, chk_window_noocc);
    }
};
inline textless_counters g_tlx;
inline thread_local bool g_tl_exact = true;   // whether the last search_textless gave the exact j (not a bound)
inline const bool g_tl_right = getenv("KATKA_TL_RIGHT") != nullptr;   // part C2b: the jump to the pairs at the right end

// C[y]: the first row of the parse's BWT whose suffix starts with phrase y (y = sigma + 1: the number of rows)
inline u64 tl_first_row(const EFRank &W, u64 y) {
    if (y > W.sigma - 1) return W.n;
    if (!W.ft.empty()) return W.ft[y].fb;
    const u64 b = W.KS.count_less(W.base_of((uint32_t)y));
    return b < W.nruns ? W.FP.access(b) : W.n;
}

// The longest suffix X[j..end) of X[begin..end) that occurs, and its number of occurrences (occ).  window: BML's
// window test, where any j with X[j-1..end) not occurring is enough.  Returns UINT64_MAX if X[begin..end) has no
// phrase boundary.
template <class TI>
inline u64 search_textless(const two_level &L, const TI &T, const pattern_parse &pi, const char *X, int64_t begin, int64_t end,
                           bool window, u64 &occ, search_counters &cnt, bool jumps = false) {
    textless_counters &tc = g_tlx;
    const int64_t K = L.K;
    const auto &S = pi.S;
    ++tc.searches; ++cnt.searches; ++(window ? tc.window_searches : tc.exact_searches);
    g_tl_exact = false;
    const int64_t idx = (int64_t)(std::upper_bound(S.begin(), S.end(), end - K) - S.begin()) - 1;
    if (idx < 0 || S[idx] < begin) { ++tc.no_boundary; occ = 0; return UINT64_MAX; }
    const int64_t s = S[idx], tl = end - s;
    const uint8_t *x = (const uint8_t *)X;
    static thread_local std::vector<uint8_t> buf;
    if ((int64_t)buf.size() < tl + 64) buf.resize(tl + 64);
    // the range of phrases starting with X[s..s+K)
    u64 r = T.R;
    const uint32_t alpha = idx > 0 ? pi.pid[idx - 1] : 0;
    if (alpha) {
        r = T.lastr.get(alpha);
        if (r == T.R) ++tc.kmer_alpha_none;
        else ++(S[idx - 1] >= begin ? tc.kmer_from_alpha : tc.kmer_from_alpha_cut);
    }
    if (r == T.R) {
        ++(idx > 0 ? tc.kmer_by_search_no_id : tc.kmer_by_search_no_alpha);
        if constexpr (!TI::whole) {             // two short copies: no K-mer search.  alpha (complete, X[S[idx-1]..S[idx]+K) within
            if (!window) ++tc.kmer_by_search_exact;   // the window) is not a phrase, so X[S[idx-1]..end) does not occur: its left
            if (window && idx > 0 && S[idx - 1] >= begin) {   // boundary + 1 is a correct j for the window test
                ++tc.alpha_noid_bound; occ = 0; return (u64)S[idx - 1] + 1;
            }
            ++tc.unresolved; occ = 0; return UINT64_MAX - 1;   // an exact search (or no alpha in the window): no verified answer
        } else {
        if (!window) ++tc.kmer_by_search_exact;
        u64 lo = 0, hi = T.R;                       // the first range whose K-mer is >= X[s..s+K)
        while (lo < hi) {
            const u64 mid = (lo + hi) / 2;
            T.chars(T.rfirst.get(mid), 0, K, buf.data()); ++tc.right_cmps;
            if (memcmp(buf.data(), x + s, K) < 0) lo = mid + 1; else hi = mid;
        }
        bool hit = false;
        if (lo < T.R) { T.chars(T.rfirst.get(lo), 0, K, buf.data()); ++tc.right_cmps; hit = !memcmp(buf.data(), x + s, K); }
        if (!hit) {
            ++tc.kmer_absent; ++tc.tail_absent; if (!window) ++tc.tail_absent_exact;
            occ = 0; return (u64)s + 1;
        }
        r = lo;
        }
    }
    u64 ya, yb; T.range(r, ya, yb);                         // [ya, yb)
    tc.range_size_sum += yb - ya;
    if (tl == K) ++tc.tail_only_kmer;
    else {
        const u64 tlen = (u64)(tl - K);
        const uint8_t *t = x + s + K;
        auto cmp = [&](u64 y) -> int {               // y[K..) against t: < 0, 0 (y starts with t), > 0
            const u64 ly = T.len(y) - K, l = std::min(ly, tlen);
            T.right(y, 0, l, buf.data()); ++tc.right_cmps;
            const int c = memcmp(buf.data(), t, l);
            if (c) return c;
            return ly >= tlen ? 0 : -1;
        };
        u64 lo = ya, hi = yb;                        // the first y with cmp(y) >= 0
        while (lo < hi) { const u64 mid = (lo + hi) / 2; if (cmp(mid) < 0) lo = mid + 1; else hi = mid; }
        if (lo == yb || cmp(lo) != 0) {
            ++tc.tail_absent; if (!window) ++tc.tail_absent_exact;
            occ = 0; return (u64)s + 1;
        }
        u64 lo2 = lo + 1, hi2 = yb;                  // the first y with cmp(y) > 0
        while (lo2 < hi2) { const u64 mid = (lo2 + hi2) / 2; if (cmp(mid) <= 0) lo2 = mid + 1; else hi2 = mid; }
        ya = lo; yb = lo2;
    }
    uint64_t plo = tl_first_row(L.pbwt, ya), phi = tl_first_row(L.pbwt, yb) - 1;
    // parse steps, with the pairs' attempts and the shortcuts over phrases as in two-level search (search2, without
    // B: the pairs' intervals map back to the parse's rows by BS)
    int64_t i = idx, noblock_above = idx - 1;
    jumps = jumps && L.has_blocks && !L.SC;
    u64 seg = 0; int segkind = 0;
    if (g_la_d && i > 0)
        for (int64_t q = i - 1, lo = std::max<int64_t>(0, i - 1 - 2 * g_la_d); q >= lo; --q) if (pi.pid[q]) L.pbwt.pf_first(pi.pid[q]);
    int64_t p1 = g_la_d && i > 0 ? std::max<int64_t>(0, i - 1 - 2 * g_la_d) - 1 : i - 1, p2 = i - 1;
    auto lookahead = [&]() {
        if (p1 > i - 1) p1 = i - 1;
        if (p2 > i - 1) p2 = i - 1;
        for (const int64_t lo = std::max<int64_t>(0, i - 1 - 2 * g_la_d); p1 >= lo; --p1) if (pi.pid[p1]) L.pbwt.pf_first(pi.pid[p1]);
        for (const int64_t lo = std::max<int64_t>(0, i - 1 - g_la_d); p2 >= lo; --p2) if (pi.pid[p2]) L.pbwt.pf_runs(pi.pid[p2]);
    };
    // the jump to the pairs at the right end (part C2b, $KATKA_TL_RIGHT): alpha = pid[idx - 1], its pair in the pattern
    // starts at t0, with d = pid[t0].  pre = 1: alpha has no successor, so it ends its pair at every occurrence: the
    // attempt starts at idx (the tail's rows that start pairs), with the side's flag true.  pre = 2: alpha's successor
    // starts with the tail, so every pair starting with d goes through alpha and the tail: the attempt starts at t0
    // with the rows of all of d's symbols, as if one pair step had been taken.
    int pre = 0; uint64_t pblo = 0, pbhi = 0;
    if (jumps && g_tl_right && idx > 0 && pi.pid[idx - 1]) {
        const int32_t t0 = pi.bstart[idx - 1];
        const uint32_t a = pi.pid[idx - 1], sa = L.succ(a);
        if (S[t0] >= begin && pi.pid[t0]) {
            if (!sa) { pre = 1; noblock_above = idx; ++tc.rj_nosucc; }
            else {
                bool starts = true;
                if (tl > K) {
                    const u64 l = (u64)(tl - K);
                    if (T.len(sa) - K < l) starts = false;
                    else { T.right(sa, 0, l, buf.data()); ++tc.right_cmps; starts = !memcmp(buf.data(), x + s + K, l); }
                }
                if (starts) {
                    u64 da, db; L.dstart.access2(pi.pid[t0], da, db);
                    pblo = tl_first_row(L.bbwt, da + 1); pbhi = tl_first_row(L.bbwt, db + 1);
                    if (pblo < pbhi) { pre = 2; noblock_above = idx; ++tc.rj_succ; } else ++tc.rj_none;
                } else ++tc.rj_notail;
            }
        } else ++tc.rj_cut;
    }
    u64 guard = 0;
    while (i > 0 && S[i - 1] >= begin && pi.pid[i - 1]) {
        if (++guard > 100000000) { fprintf(stderr, "textless: no progress: idx %ld i %ld noblock_above %ld plo %lu phi %lu begin %ld end %ld\n", (long)idx, (long)i, (long)noblock_above, plo, phi, (long)begin, (long)end); abort(); }
        if (g_la_d) lookahead();
        // (at idx, the pattern's phrase pid[idx] runs past the window's end, so cont[idx] says nothing about the window)
        if (jumps && i <= noblock_above && (!pi.cont[i] || (pre && i == idx))) {
            (segkind ? cnt.pe_inside : cnt.pe_entry) += seg; seg = 0;
            ++cnt.b_attempts;
            uint64_t blo, bhi;
            int64_t cur = i, taken = 0;
            uint64_t sblo = 0, sbhi = 0; int64_t scur = i;
            if (pre == 2) { blo = pblo; bhi = pbhi; cur = pi.bstart[i - 1]; taken = 1; ++cnt.block_steps; }
            else { blo = L.BS.count_less(plo); bhi = L.BS.count_less(phi + 1); }
            const bool first_flag = pre == 1;
            pre = 0;
            while (cur > 0 && blo < bhi) {
                int32_t t0 = pi.bstart[cur - 1];
                if (S[t0] < begin || !pi.pid[t0]) break;
                const bool capped = L.maxblock && pi.pid[cur] == L.succ(pi.pid[cur - 1]);
                const bool plus = capped || pi.pid[cur] > L.succ(pi.pid[cur - 1]) || (first_flag && cur == idx && !L.sided);
                uint32_t yy = L.sym(pi.pid[t0], capped ? (uint64_t)L.maxblock : (uint64_t)(cur - 1 - t0), plus);
                if (!yy) { ++cnt.b_nosym; break; }
                uint64_t o1, o2;
                L.bbwt.lf2(yy, blo, bhi, o1, o2);
                ++cnt.block_steps;
                if (o1 >= o2) { ++cnt.b_fail; break; }
                if (cur != i && pi.real[cur]) { sblo = blo; sbhi = bhi; scur = cur; }
                blo = o1; bhi = o2; cur = t0; ++taken;
            }
            if (taken >= 1 && !L.can_continue(pi.pid[cur])) {     // exact (Lemma "backup", first case)
                plo = L.BS.access(blo); phi = L.BS.access(bhi - 1);
                i = cur; noblock_above = i;
                ++cnt.b_exact; segkind = 1;
                continue;
            }
            if (taken >= 1 && scur != i) {                      // back up to the last exact interval before the last block
                plo = L.BS.access(sblo); phi = L.BS.access(sbhi - 1);
                i = scur;
                ++cnt.b_backups;
            }
            noblock_above = pi.bstart[i - 1];
            segkind = 1;
        }
        pre = 0;                                                // (a jump not taken at idx is dropped)
        if (L.SC && i < idx) {                                  // [plo, phi] starts with the complete phrase pid[i]
            const parse_shortcuts &SC = *L.SC;
            const uint64_t q = SC.find(pi.pid[i]);
            if (q != ~0ull) {
                ++cnt.sc_met;
                const int64_t k = (int64_t)SC.K.get(q);
                if (i - k >= 0 && S[i - k] >= begin && pi.zc[i] == pi.zc[i - k] && pi.fp(SC, i - k, i) == SC.H[q]) {
                    const uint64_t a = SC.A.get(q), lo = a + SC.surv(q, plo), hi = a + SC.surv(q, phi + 1);
                    if (lo < hi) { ++cnt.sc_taken; cnt.sc_crossed += k; plo = lo; phi = hi - 1; i -= k; continue; }
                    ++cnt.sc_failed;
                } else ++cnt.sc_mismatch;
            }
        }
        uint64_t o1, o2;
        L.pbwt.lf2(pi.pid[i - 1], plo, phi + 1, o1, o2);
        ++cnt.parse_steps; ++seg;
        if (o1 >= o2) { if (i == idx) ++tc.alpha_step_fail; break; }
        plo = o1; phi = o2 - 1; --i;
    }
    (segkind ? cnt.pe_end : cnt.pe_entry) += seg;
    // the left end
    const int64_t si = S[i];
    const u64 cap = (u64)(si - begin);
    ++tc.left;
    g_tl_exact = true;
    if (cap == 0) { ++tc.left_at_cap; occ = phi - plo + 1; return (u64)si; }
    static thread_local std::vector<uint32_t> ys;
    ys.clear();
    for (u64 jr = T.hstart.pred_index(plo); jr < T.hruns && T.hstart.access(jr) <= phi; ++jr) ys.push_back((uint32_t)T.hsym.get(jr));
    tc.left_runs_sum += ys.size();
    std::sort(ys.begin(), ys.end()); ys.erase(std::unique(ys.begin(), ys.end()), ys.end());
    if (!ys.empty() && ys[0] == 0) ys.erase(ys.begin());       // the parse's start: nothing precedes it
    tc.left_distinct_sum += ys.size(); tc.left_distinct_max = std::max<u64>(tc.left_distinct_max, ys.size()); tc.hist(ys.size());
    static thread_local std::vector<u64> agree;
    agree.assign(ys.size(), 0);
    u64 best = 0;
    for (size_t q = 0; q < ys.size(); ++q) {
        const u64 y = ys[q], ly = T.len(y) - K, lim = std::min(ly, cap);
        u64 a = 0;
        while (a < lim) {                            // compare leftward in chunks of 64 characters
            const u64 c = std::min<u64>(64, lim - a);
            T.left(y, ly - a - c, ly - a, buf.data());
            u64 t = 0;
            while (t < c && buf[c - 1 - t] == x[si - 1 - (int64_t)(a + t)]) ++t;
            tc.left_cmps_chars += t + (t < c);
            a += t;
            if (t < c) break;
        }
        if (a == ly && ly < cap) ++tc.bug_full_agree;
        agree[q] = a; best = std::max(best, a);
    }
    if (best == cap) ++tc.left_at_cap;
    if (best == 0) { occ = phi - plo + 1; return (u64)si; }
    occ = 0;
    for (size_t q = 0; q < ys.size(); ++q) if (agree[q] == best) { uint64_t o1, o2; L.pbwt.lf2(ys[q], plo, phi + 1, o1, o2); occ += o2 - o1; }
    return (u64)si - best;
}

}  // namespace katka
