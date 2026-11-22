// A move structure for the text-level steps of two-level search (2026-11-12, "answer on the STPD
// profile", A): backward steps on the BWT of S$ as in Movi (Nishimoto and Tabei's move structure; Zakeri et al.),
// with the interval kept as (run, offset) at both ends.
//   rec[q]   run q of the BWT, after splitting runs longer than 2^16 - 1: its symbol (3 bits), its length (16),
//            and LF of its first row as (destination run (29 bits), offset in it (16)); one 64-bit word;
//   starts   the runs' first rows (Elias-Fano): positions <-> (run, offset), only where search2 changes levels;
//   runs_of  for each symbol, the indices of its runs (Elias-Fano): the next / previous run with a symbol,
//            when a short scan of the neighbouring runs does not find one.
// A step with c on [sp, ep]: move sp forward to the first row with c (its own run, else the first c-run after it),
// ep backward to the last (its own run, else the last c-run before it); empty if they cross; then LF both:
// LF(row) = LF(run's first row) + offset, then forward over whole runs while the offset exceeds the run's length.
// Exactly the interval of backward search (checked against the RLCSA by `find --move-check`).
#pragma once
#include "rlbwt.hpp"
#include "coro.hpp"
#include <functional>
namespace katka {
struct move_index {
    u64 n = 0, r = 0;
    std::vector<uint64_t> rec;
    EliasFano starts;
    std::vector<EliasFano> runs_of;      // per symbol
    uint8_t sym_of[256], byte_of[8];
    uint32_t nsym = 0;
    static constexpr u64 MAXLEN = 65535, SCAN = 8;
    move_index() { std::fill(sym_of, sym_of + 256, 255); std::fill(byte_of, byte_of + 8, 0); }

    inline uint32_t sym(u64 q) const { return (uint32_t)(rec[q] & 7); }
    inline u64 len(u64 q) const { return (rec[q] >> 3) & 0xffff; }
    inline u64 off(u64 q) const { return (rec[q] >> 19) & 0xffff; }
    inline u64 dest(u64 q) const { return rec[q] >> 35; }

    void build(const rlbwt &B) {
        n = B.n;
        std::vector<u64> st, lf; std::vector<uint8_t> sy, ln;
        std::vector<uint32_t> lens;
        u64 cnt[256] = {0};
        for (u64 q = 0; q < B.R; ++q) {
            const u64 s = B.run_start(q), e = q + 1 < B.R ? B.run_start(q + 1) : B.n;
            const unsigned char c = B.head(q);
            if (sym_of[c] == 255) { if (nsym == 8) die("move structure: more than 8 symbols"); }
            for (u64 a = s; a < e; a += MAXLEN) {
                const u64 l = std::min(MAXLEN, e - a);
                st.push_back(a); lens.push_back((uint32_t)l); sy.push_back(c);
                lf.push_back(B.C[c] + cnt[c]); cnt[c] += l;
            }
        }
        // symbols in byte order
        for (int c = 0; c < 256; ++c) if (cnt[c]) { if (nsym == 8) die("move structure: more than 8 symbols"); sym_of[c] = (uint8_t)nsym; byte_of[nsym++] = (uint8_t)c; }
        r = st.size();
        if (r >= (1ull << 29)) die("move structure: too many runs");
        rec.assign(r, 0);
        // destinations: for each symbol, LF of its runs' first rows increases, so one pointer walks the runs
        for (uint32_t s = 0; s < nsym; ++s) {
            const unsigned char c = byte_of[s];
            u64 p = (u64)(std::upper_bound(st.begin(), st.end(), B.C[c]) - st.begin()) - 1;
            for (u64 q = 0; q < r; ++q) {
                if (sy[q] != c) continue;
                const u64 v = lf[q];
                while (p + 1 < r && st[p + 1] <= v) ++p;
                const u64 o = v - st[p];
                if (o >= lens[p]) die("move structure: internal error (offset)");
                rec[q] = (uint64_t)s | ((uint64_t)lens[q] << 3) | ((uint64_t)o << 19) | ((uint64_t)p << 35);
            }
        }
        starts.build(st, n);
        runs_of.assign(nsym, EliasFano());
        for (uint32_t s = 0; s < nsym; ++s) {
            std::vector<u64> v;
            for (u64 q = 0; q < r; ++q) if (sym(q) == s) v.push_back(q);
            runs_of[s].build(v, r);
        }
    }
    u64 bytes() const { u64 b = rec.size() * 8 + starts.bytes(); for (auto &e : runs_of) b += e.bytes(); return b; }
    void save(const std::string &f) const {
        FILE *o = open_or_die(f, "wb"); wmagic(o, "KTMOVE01");
        wr(o, n); wr(o, r); wr(o, nsym); fwrite(sym_of, 1, 256, o); fwrite(byte_of, 1, 8, o);
        wvec(o, rec); starts.save(o); for (auto &e : runs_of) e.save(o);
        fclose(o);
    }
    void load(const std::string &f) {
        FILE *i = open_or_die(f, "rb"); rmagic(i, "KTMOVE01", f);
        rd(i, n); rd(i, r); rd(i, nsym);
        if (fread(sym_of, 1, 256, i) != 256 || fread(byte_of, 1, 8, i) != 8) die("short read " + f);
        rvec(i, rec); starts.load(i); runs_of.assign(nsym, EliasFano()); for (auto &e : runs_of) e.load(i);
        fclose(i);
    }

    // an interval as (run, offset) of its first and last rows
    struct cursor { u64 rs = 0, os = 0, re = 0, oe = 0; };
    cursor full() const { return {0, 0, r - 1, len(r - 1) - 1}; }
    cursor at(u64 sp, u64 ep) const {             // [sp, ep), nonempty
        if (sp == 0 && ep == n) return full();
        cursor c;
        c.rs = starts.pred_index(sp); c.os = sp - starts.access(c.rs);
        c.re = starts.pred_index(ep - 1); c.oe = ep - 1 - starts.access(c.re);
        return c;
    }
    void rows(const cursor &c, u64 &sp, u64 &ep) const { sp = starts.access(c.rs) + c.os; ep = starts.access(c.re) + c.oe + 1; }
    inline void lf(u64 &q, u64 &o) const {
        o += off(q); q = dest(q);
        while (o >= len(q)) { o -= len(q); ++q; }
    }
    // the cursors of all t-mers over A/C/G/T (t = tk), read right to left from the full interval; rs = UINT64_MAX:
    // the t-mer does not occur.  Built at load (make_table), not stored.
    int tk = 0; std::vector<cursor> tab;
    static inline int code2(unsigned char c) { return c == 'A' ? 0 : c == 'C' ? 1 : c == 'G' ? 2 : c == 'T' ? 3 : -1; }
    void make_table(int t) {
        tk = t; tab.assign((size_t)1 << (2 * t), cursor{UINT64_MAX, 0, 0, 0});
        static const char B4[4] = {'A', 'C', 'G', 'T'};
        // key: the t-mer read left to right, 2 bits per character, the leftmost in the top bits; the d-th step
        // (from the right, d = 0 first) is the character at bits 2d
        std::function<void(int, u64, cursor)> rec_ = [&](int d, u64 key, cursor c) {
            if (d == t) { tab[key] = c; return; }
            for (int a = 0; a < 4; ++a) { cursor x = c; if (step(x, (unsigned char)B4[a])) rec_(d + 1, key | ((u64)a << (2 * d)), x); }
        };
        rec_(0, 0, full());
    }
    // backward step with byte c; false (c unchanged) if the interval becomes empty.  Y (plain search, chopped and
    // interleaved; g_co set): once both ends' destination runs are known, prefetch their records and yield to the next
    // piece before fast-forwarding (the destination records carry about 97% of plain search's L3 misses).
    template <bool Y = false>
    inline bool step(cursor &c, unsigned char ch) const {
        const uint32_t s = sym_of[ch];
        if (s == 255) return false;
        u64 a = c.rs, oa = c.os, e = c.re, oe = c.oe;
        if (sym(a) != s) {
            u64 b = a + 1, k = 0;
            while (k < SCAN && b <= e && sym(b) != s) { ++b; ++k; }
            if (b > e) return false;
            if (sym(b) != s) {
                const u64 t = runs_of[s].count_less(b);
                if (t == runs_of[s].n) return false;
                b = runs_of[s].access(t);
                if (b > e) return false;
            }
            a = b; oa = 0;
        }
        if (sym(e) != s) {                         // a <= e and run a has s, so a c-run at or before e exists
            u64 b = e - 1, k = 0;
            while (k < SCAN && b > a && sym(b) != s) { --b; ++k; }
            if (sym(b) != s) b = runs_of[s].access(runs_of[s].count_less(b) - 1);
            e = b; oe = len(b) - 1;
        }
        if constexpr (Y) {
            oa += off(a); a = dest(a); oe += off(e); e = dest(e);
            __builtin_prefetch(&rec[a]); __builtin_prefetch(&rec[e]);
            g_co->yield();
            while (oa >= len(a)) { oa -= len(a); ++a; }
            while (oe >= len(e)) { oe -= len(e); ++e; }
        } else {
            lf(a, oa); lf(e, oe);
        }
        c = {a, oa, e, oe};
        return true;
    }
};
}  // namespace katka
