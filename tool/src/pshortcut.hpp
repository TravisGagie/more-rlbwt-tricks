// Shortcuts over phrases on the two-level index (2026-11-08 evening, item 3): Reportable Shortcuts'
// idea on the parse instead of the text.
//
// A shortcut is attached to a phrase w (its entrance) and crosses a stretch alpha = a_1 .. a_k of k >= 2 phrases
// that precedes w in many of w's occurrences (in text order alpha w).  Backward search over the parse holds the
// parse interval [plo, phi] of a pattern suffix Z that starts with the complete phrase w, so [plo, phi] lies in
// w's F-range [C_w, C_w + occ(w)).  If the pattern's k phrases before Z are alpha, the interval of alpha Z is
//     [A + s(plo), A + s(phi + 1))
// where A is the first row of alpha w's interval and s(y) is the number of rows of [C_w, y) whose suffix is
// preceded by alpha (the "survivors"): the rows of alpha w and the surviving rows of w are in the same order
// (they differ by the common prefix alpha).  One step crosses k phrases.  If the interval is empty, alpha Z does
// not occur: the shortcut fails midway and the search goes on with ordinary parse steps from [plo, phi].
//
// Stored per shortcut: k, A, C_w, a 64-bit fingerprint of alpha (Karp-Rabin over the phrase ids, mod 2^61 - 1;
// the pattern's is computed from prefix fingerprints), and which of w's rows are exceptions: the survivors or
// the others, whichever are fewer, as one Elias-Fano list over all parse rows (with the count before C_w).
// Entrances: a bitvector over the phrase ids with rank.
//
// Placement (pshortcut_build.hpp): from the reference's parse (every T-th phrase of the first sequence gets the
// stretch of the T phrases before it), or by heavy paths (from w, follow the most frequent predecessor while at
// least a fraction theta of w's occurrences are preceded by the path so far).
#pragma once
#include "bits.hpp"

namespace katka {

struct parse_shortcuts {
    static constexpr uint64_t MOD = (1ull << 61) - 1, BASE = 0x2545f4914f6cdd1dull % ((1ull << 61) - 1);
    uint64_t kr = BASE;             // the fingerprints' base (index --kr-base); stored only if not BASE (file KTPSC002)
    static inline uint64_t mulmod(uint64_t a, uint64_t b) {
        unsigned __int128 z = (unsigned __int128)a * b;
        uint64_t lo = (uint64_t)(z & MOD), hi = (uint64_t)(z >> 61);
        uint64_t s = lo + hi;
        return s >= MOD ? s - MOD : s;
    }
    static inline uint64_t addmod(uint64_t a, uint64_t b) { uint64_t s = a + b; return s >= MOD ? s - MOD : s; }
    static inline uint64_t submod(uint64_t a, uint64_t b) { return a >= b ? a - b : a + MOD - b; }

    std::string spec;               // how they were placed, e.g. "heavy:0.9" or "ref:16"
    uint64_t m = 0;                 // phrases of the index
    uint32_t sigma = 0, kmax = 0;
    uint64_t nsc = 0;               // shortcuts
    BitRank has;                    // over ids 0 .. sigma: w has a shortcut
    PackedInts K, A, Cw, base;      // per shortcut, in id order
    std::vector<uint64_t> H;        // fingerprints of alpha
    std::vector<uint64_t> flag;     // bit q: shortcut q's exceptions are its survivors (else its non-survivors)
    EliasFano X;                    // exception rows
    std::vector<uint64_t> pw;       // kr^j, j = 0 .. kmax (not stored)

    void prepare() { pw.assign(kmax + 1, 1); for (uint32_t j = 1; j <= kmax; ++j) pw[j] = mulmod(pw[j - 1], kr); }
    inline bool flagged(uint64_t q) const { return (flag[q >> 6] >> (q & 63)) & 1; }
    // the shortcut of w, or ~0
    inline uint64_t find(uint32_t w) const { return w <= sigma && has.get(w) ? has.rank1(w) : ~0ull; }
    // survivors of shortcut q in [C_w, y)
    inline uint64_t surv(uint64_t q, uint64_t y) const {
        uint64_t e = X.count_less(y) - base.get(q);
        return flagged(q) ? e : (y - Cw.get(q)) - e;
    }
    uint64_t bytes() const {
        return has.bytes() + K.bytes() + A.bytes() + Cw.bytes() + base.bytes() + H.size() * 8 + flag.size() * 8 + X.bytes();
    }
    void save(const std::string &path) const {
        FILE *f = open_or_die(path, "wb");
        wmagic(f, kr == BASE ? "KTPSC001" : "KTPSC002");
        uint64_t sl = spec.size(); wr(f, sl); fwrite(spec.data(), 1, sl, f);
        wr(f, m); wr(f, sigma); wr(f, kmax); wr(f, nsc);
        if (kr != BASE) wr(f, kr);
        has.save(f); K.save(f); A.save(f); Cw.save(f); base.save(f); wvec(f, H); wvec(f, flag); X.save(f);
        fclose(f);
    }
    void load(const std::string &path) {
        FILE *f = open_or_die(path, "rb");
        char mg[8];
        if (fread(mg, 1, 8, f) != 8 || (memcmp(mg, "KTPSC001", 8) && memcmp(mg, "KTPSC002", 8))) die(path + ": not a KTPSC001/2 file");
        uint64_t sl; rd(f, sl); spec.resize(sl); if (fread(&spec[0], 1, sl, f) != sl) die(path + ": truncated");
        rd(f, m); rd(f, sigma); rd(f, kmax); rd(f, nsc);
        kr = BASE; if (!memcmp(mg, "KTPSC002", 8)) rd(f, kr);
        has.load(f); K.load(f); A.load(f); Cw.load(f); base.load(f); rvec(f, H); rvec(f, flag); X.load(f);
        fclose(f);
        prepare();
    }
};

}  // namespace katka
