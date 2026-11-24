// Two-level indexing with blocks (Section 5 of the paper, "Two-level indexing with a syncmer parse" and
// "Blocks"), on the text of both strands.
//
// The text S (with its terminator $) is parsed at the starts of closed syncmers and at position 0.  A
// k-mer over A/C/G/T is a closed syncmer if its smallest s-mer under the order g is its first or its last
// (ties included).  Unlike More RLBWT Tricks, which indexed T and the reversed collection T', g is
// canonical under reverse complement: g(gamma) is a hash of the smaller of the 2-bit codes of gamma and of
// rc(gamma).  The s-mers of rc(kappa) are the reverse complements of those of kappa, in reverse order, so
// kappa is a closed syncmer if and only if rc(kappa) is.  Consequences:
//   - whether a k-mer is a closed syncmer still does not depend on its context, so the parse is still
//     prefix-free and the order lemma (Lemma 6) holds unchanged: its proof uses nothing about g;
//   - the syncmer starts of rc(P) are m - k - q for the starts q of P, in reverse order, and the phrases
//     of rc(P) are the reverse complements of P's, so one parse of the pattern serves the backward search
//     on P and the forward search (backward on rc(P)), and one dictionary, storing for each phrase of S
//     its id and the id of its reverse complement, serves both strands.
//
// Phrase i is S[b_i .. b_{i+1} + k), the last one S[b_last .. n) (it ends with $); phrases are numbered
// 1..sigma in byte order ($ = 1 < A < C < G < T < X), and the parse is their sequence followed by 0.
//   B     marks the rows of the BWT of S whose suffixes start at boundaries; the u-th 1 (from 0) is parse
//         row u + 1.  It is the union of the intervals of the distinct syncmers (plus the row of position
//         0), stored as runs.
//   pbwt  the BWT of the parse, stored like an RLCSA (EFRank).
// Blocks: succ(x) is the smallest phrase that follows x in the parse (or, with --continuation frequent,
// the most frequent one).  A block is a phrase d that is not a continuation followed by its j >= 0
// continuations; its symbol is (d, j, side).  Symbols are ordered by d, then - before +, -'s by j
// ascending and +'s by j descending, so suffixes starting at blocks are in the same order in the block
// sequence as in the parse (Lemma "blockorder" and "Other continuations").
//   BS    marks the parse rows that start blocks (and row 0);  bbwt  the BWT of the block sequence.
#pragma once
#include "search.hpp"
#include "pshortcut.hpp"
#ifdef KATKA_NS
#include "ns_count.hpp"
#endif
#include "prof.hpp"

namespace katka {

inline uint64_t mix64(uint64_t x) {
    x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ull;
    x ^= x >> 27; x *= 0x94d049bb133111ebull;
    x ^= x >> 31;
    return x;
}

// Calls f(q) for each start q of a closed syncmer of x[0 .. len), in order.  The order g is a hash of the
// smaller of the codes of an s-mer and of its reverse complement (0 marks s-mers with other characters).
// Window minima by van Herk / Gil-Werman, in chunks.
template <class F> void for_each_syncmer(const uint8_t *x, uint64_t len, int K, int SM, F f) {
    static int8_t code[256];
    static bool init = false;
    if (!init) {
        for (int c = 0; c < 256; ++c) code[c] = -1;
        code['A'] = 0; code['C'] = 1; code['G'] = 2; code['T'] = 3;
        init = true;
    }
    if (len < (uint64_t)K) return;
    const uint64_t W = (uint64_t)(K - SM + 1);
    const uint64_t nq = len - K + 1;
    const uint64_t CH = W * 256;
    const uint64_t mask = SM == 32 ? ~0ull : (1ull << (2 * SM)) - 1;
    static thread_local std::vector<uint64_t> g, pre, suf;
    g.resize(CH + W); pre.resize(CH + W); suf.resize(CH + W);
    for (uint64_t c0 = 0; c0 < nq; c0 += CH) {
        const uint64_t cq = std::min(CH, nq - c0), ns = cq + W - 1;
        uint64_t smer = 0, rsmer = 0;
        int64_t bad = -1;
        for (int64_t i = 0; i < SM - 1; ++i) {
            int cc = code[x[c0 + i]];
            if (cc < 0) { bad = i; cc = 0; }
            smer = ((smer << 2) | (uint64_t)cc) & mask;
            rsmer = (rsmer >> 2) | ((uint64_t)(3 - cc) << (2 * (SM - 1)));
        }
        for (uint64_t t = 0; t < ns; ++t) {
            int64_t i = (int64_t)t + SM - 1;
            int cc = code[x[c0 + i]];
            if (cc < 0) { bad = i; cc = 0; }
            smer = ((smer << 2) | (uint64_t)cc) & mask;
            rsmer = (rsmer >> 2) | ((uint64_t)(3 - cc) << (2 * (SM - 1)));
            uint64_t h = mix64(smer < rsmer ? smer : rsmer);
            g[t] = bad >= (int64_t)t ? 0 : (h >> 1) + 1;
        }
        for (uint64_t b0 = 0; b0 < ns; b0 += W) {
            uint64_t b1 = std::min(ns, b0 + W), mn = ~0ull;
            for (uint64_t t = b0; t < b1; ++t) { mn = g[t] < mn ? g[t] : mn; pre[t] = mn; }
            mn = ~0ull;
            for (uint64_t t = b1; t-- > b0;) { mn = g[t] < mn ? g[t] : mn; suf[t] = mn; }
        }
        for (uint64_t q = 0; q < cq; ++q) {
            uint64_t a = suf[q], bb = pre[q + W - 1], mn = a < bb ? a : bb;
            if (mn && (mn == g[q] || mn == g[q + W - 1])) f(c0 + q);
        }
    }
}

// hash of a byte string (never 0)
inline uint64_t phrase_hash(const uint8_t *p, uint64_t len) {
    uint64_t hv = mix64(len ^ 0x9e3779b97f4a7c15ull), i = 0;
    for (; i + 8 <= len; i += 8) { uint64_t w; memcpy(&w, p + i, 8); hv = mix64(hv ^ w) + i; }
    uint64_t w = 0;
    memcpy(&w, p + i, len - i);
    hv = mix64(hv ^ w ^ (len << 56));
    return hv ? hv : 1;
}

// The phrase dictionary: the 64-bit hashes of the phrases of S over A/C/G/T, sorted and split into
// buckets by their top hb bits (about 8 per bucket); each slot stores the other 64 - hb bits (so the whole
// hash is checked), the phrase's id and the id of its reverse complement.
struct PhraseDict {
    uint32_t hb = 1;
    uint64_t size = 0;
    std::vector<uint32_t> bstart;
    PackedInts low, idf, idr;           // as stored in the file; in memory after build(), dropped by load()
    // in memory: each slot's low hash bits, id and rc id in one packed record of w_lo + w_f + wr bits, so a lookup
    // touches one or two adjacent cache lines instead of three scattered ones (2026-11-12; the file is unchanged)
    uint32_t w_lo = 1, w_f = 1, w_r = 1, wrec = 3;
    std::vector<uint64_t> rec;
    static inline uint64_t bits(const std::vector<uint64_t> &d, uint64_t pos, uint32_t w) {
        const uint64_t k = pos >> 6, o = pos & 63;
        uint64_t x = d[k] >> o;
        if (o + w > 64) x |= d[k + 1] << (64 - o);
        return w == 64 ? x : x & ((1ull << w) - 1);
    }
    static inline void setbits(std::vector<uint64_t> &d, uint64_t pos, uint32_t w, uint64_t x) {
        const uint64_t k = pos >> 6, o = pos & 63;
        d[k] |= x << o;
        if (o + w > 64) d[k + 1] |= x >> (64 - o);
    }
    void make_rec() {
        w_lo = low.w; w_f = idf.w; w_r = idr.w; wrec = w_lo + w_f + w_r;
        rec.assign((size * wrec + 63) / 64 + 2, 0);
        for (uint64_t t = 0; t < size; ++t) {
            const uint64_t p = t * wrec;
            setbits(rec, p, w_lo, low.get(t)); setbits(rec, p + w_lo, w_f, idf.get(t)); setbits(rec, p + w_lo + w_f, w_r, idr.get(t));
        }
    }
    void unpack() {                      // low, idf, idr from rec (for save after load)
        low.init(size, w_lo); idf.init(size, w_f); idr.init(size, w_r);
        for (uint64_t t = 0; t < size; ++t) {
            const uint64_t p = t * wrec;
            low.set(t, bits(rec, p, w_lo)); idf.set(t, bits(rec, p + w_lo, w_f)); idr.set(t, bits(rec, p + w_lo + w_f, w_r));
        }
    }
    inline uint64_t idf_at(uint64_t t) const { return bits(rec, t * wrec + w_lo, w_f); }
    void build(std::vector<std::pair<uint64_t, std::pair<uint32_t, uint32_t>>> v) {   // (hash, (id, rc id))
        std::sort(v.begin(), v.end());
        for (size_t k = 1; k < v.size(); ++k)
            if (v[k].first == v[k - 1].first) throw std::runtime_error("two phrases with the same hash");
        size = v.size(); hb = 1;
        while (hb < 32 && (8ull << hb) < size) ++hb;
        uint64_t nb = 1ull << hb, k = 0;
        bstart.assign(nb + 1, 0);
        for (uint64_t bk = 0; bk < nb; ++bk) {
            bstart[bk] = (uint32_t)k;
            while (k < v.size() && (v[k].first >> (64 - hb)) == bk) ++k;
        }
        bstart[nb] = (uint32_t)k;
        std::vector<uint64_t> lw(size), f(size), r(size);
        for (size_t t = 0; t < size; ++t) { lw[t] = v[t].first & ((1ull << (64 - hb)) - 1); f[t] = v[t].second.first; r[t] = v[t].second.second; }
        low.build(lw); idf.build(f); idr.build(r);
        make_rec();
    }
    inline void prefetch_bucket(uint64_t hv) const { __builtin_prefetch(&bstart[hv >> (64 - hb)]); }
    inline void prefetch(uint64_t hv) const {
        const uint64_t p = ((uint64_t)bstart[hv >> (64 - hb)] * wrec) >> 6;
        __builtin_prefetch(&rec[p]); __builtin_prefetch(&rec[p] + 8);
    }
    // the slot of hv and its ids; false if hv is not a phrase's hash
    inline bool lookup(uint64_t hv, uint32_t &f, uint32_t &r) const {
        uint64_t bk = hv >> (64 - hb), lo = hv & ((1ull << (64 - hb)) - 1);
        for (uint64_t a = bstart[bk], b = bstart[bk + 1]; a < b; ++a) {
            const uint64_t p = a * wrec, x = bits(rec, p, w_lo);
            if (x == lo) { f = (uint32_t)bits(rec, p + w_lo, w_f); r = (uint32_t)bits(rec, p + w_lo + w_f, w_r); return true; }
            if (x > lo) break;
        }
        return false;
    }
    uint64_t bytes() const { return bstart.size() * 4 + rec.size() * 8 + 24; }
    void save(FILE *f) const {
        if (low.n != size) { PhraseDict c = *this; c.unpack(); c.save(f); return; }
        wr_(f);
    }
    void wr_(FILE *f) const { wr(f, hb); wr(f, size); wvec(f, bstart); low.save(f); idf.save(f); idr.save(f); }
    void load(FILE *f) {
        rd(f, hb); rd(f, size); rvec(f, bstart); low.load(f); idf.load(f); idr.load(f);
        make_rec(); low = PackedInts(); idf = PackedInts(); idr = PackedInts();
    }
};

// The block-start rows: Elias-Fano or run-length coded, whichever is smaller.
struct BlockStarts {
    uint8_t rl = 0;
    uint64_t alt_bytes = 0;
    EliasFano ef;
    RLBits r;
    void build(const std::vector<uint64_t> &rows, uint64_t universe) {
        std::vector<std::pair<uint64_t, uint64_t>> runs;
        for (uint64_t x : rows) {
            if (!runs.empty() && runs.back().first + runs.back().second == x) ++runs.back().second;
            else runs.push_back({x, 1});
        }
        r.build(runs, universe);
        ef.build(rows, universe);
        rl = r.bytes() < ef.bytes();
        alt_bytes = rl ? ef.bytes() : r.bytes();
        if (rl) ef = EliasFano(); else r = RLBits();
    }
    inline uint64_t count_less(uint64_t x) const { return rl ? r.rank1(x) : ef.count_less(x); }
    inline uint64_t access(uint64_t k) const { return rl ? r.select1(k) : ef.access(k); }
    uint64_t bytes() const { return rl ? r.bytes() : ef.bytes(); }
    void save(FILE *f) const { wr(f, rl); if (rl) r.save(f); else ef.save(f); }
    void load(FILE *f) { rd(f, rl); if (rl) r.load(f); else ef.load(f); }
};

struct two_level {
    int32_t K = 51, SM = 12;
    uint64_t n = 0;          // rows of the character-level BWT
    uint64_t m = 0;          // phrases; the parse has m + 1 rows
    uint32_t sigma = 0;      // distinct phrases, ids 1..sigma
    uint64_t nblocks = 0;    // blocks; the block BWT has nblocks + 1 rows
    uint64_t nsym = 0;       // distinct block symbols, ids 1..nsym
    uint8_t has_blocks = 1;
    RLBits B;
    EFRank pbwt;
    BlockStarts BS;
    EFRank bbwt;
    PackedInts succp;               // succ(x), x = 0..sigma (0: none)
    EliasFano dstart;               // symbols of d are dstart[d] .. dstart[d+1]-1 (0-based)
    PackedInts jv;                  // j of each symbol, in symbol order within each d
    uint8_t sided = 0;              // 1: symbols have sides (sidev); 0: all +
    uint32_t maxblock = 0;          // --max-block B: a block also ends after B phrases (0: no cap); stored in KTLVL002 files
    std::vector<uint64_t> sidev;
    std::vector<uint64_t> ccont;    // bit x: phrase x is a continuation somewhere in the parse
    PhraseDict dict;
    const parse_shortcuts *SC = nullptr;   // shortcuts over phrases (find --pshortcuts; not part of the file)

    inline uint32_t succ(uint32_t x) const { return (uint32_t)succp.get(x); }
    inline bool can_continue(uint32_t x) const { return (ccont[x >> 6] >> (x & 63)) & 1; }
    inline bool sym_plus(uint64_t k) const { return !sided || ((sidev[k >> 6] >> (k & 63)) & 1); }
    static inline uint64_t sym_key(uint64_t j, bool plus) { return plus ? (1ull << 32) | (0xffffffffull - j) : j; }
    // the symbol (d, j, side) (1-based), or 0 if it does not occur
    inline uint32_t sym(uint32_t d, uint64_t j, bool plus) const {
        if (!sided && !plus) return 0;
        uint64_t a0, b0;
        dstart.access2(d, a0, b0);
        const uint64_t want = sym_key(j, plus);
        uint64_t a = a0, b = b0;
        while (a < b) {
            uint64_t mid = (a + b) >> 1;
            if (sym_key(jv.get(mid), sym_plus(mid)) < want) a = mid + 1; else b = mid;
        }
        return (a < b0 && sym_key(jv.get(a), sym_plus(a)) == want) ? (uint32_t)a + 1 : 0;
    }
    uint64_t bytes_parse() const { return B.bytes() + pbwt.bytes() + dict.bytes(); }
    uint64_t bytes_blocks() const { return succp.bytes() + ccont.size() * 8 + BS.bytes() + bbwt.bytes() + dstart.bytes() + jv.bytes() + sidev.size() * 8; }

    void save(const std::string &path) const {
        FILE *f = open_or_die(path, "wb");
        wmagic(f, maxblock ? "KTLVL002" : "KTLVL001");
        wr(f, K); wr(f, SM); wr(f, n); wr(f, m); wr(f, sigma); wr(f, nblocks); wr(f, nsym); wr(f, has_blocks); wr(f, sided);
        if (maxblock) wr(f, maxblock);
        B.save(f); pbwt.save(f); dict.save(f);
        if (has_blocks) { BS.save(f); bbwt.save(f); succp.save(f); dstart.save(f); jv.save(f); wvec(f, ccont); wvec(f, sidev); }
        fclose(f);
    }
    // parse_only: keep B and the dictionary (for STPD over the parse); the parse BWT and the blocks are read
    // and dropped
    void load(const std::string &path, bool parse_only = false) {
        FILE *f = open_or_die(path, "rb");
        char mg[8];
        if (fread(mg, 1, 8, f) != 8 || (memcmp(mg, "KTLVL001", 8) && memcmp(mg, "KTLVL002", 8))) die(path + ": not a KTLVL001/2 file (rebuild it)");
        rd(f, K); rd(f, SM); rd(f, n); rd(f, m); rd(f, sigma); rd(f, nblocks); rd(f, nsym); rd(f, has_blocks); rd(f, sided);
        maxblock = 0; if (!memcmp(mg, "KTLVL002", 8)) rd(f, maxblock);
        B.load(f); pbwt.load(f); dict.load(f);
        if (parse_only) { pbwt = EFRank(); has_blocks = 0; fclose(f); return; }
        if (has_blocks) { BS.load(f); bbwt.load(f); succp.load(f); dstart.load(f); jv.load(f); rvec(f, ccont); rvec(f, sidev); }
        fclose(f);
        if (!getenv("KATKA_NO_FIRST_RUN")) { pbwt.build_first(); if (has_blocks) bbwt.build_first(); }   // the first-run tables (KATKA_NO_FIRST_RUN: none)
    }
};

// The parse of a pattern against the dictionary.
struct pattern_parse {
    std::vector<int64_t> S;         // syncmer starts
    std::vector<uint32_t> pid;      // pid[i]: id of P[S[i] .. S[i+1]+K), 0 if not a phrase (or i is last)
    std::vector<uint8_t> cont;      // cont[i]: phrase i continues phrase i-1's block (with a cap, within B phrases)
    std::vector<uint8_t> real;      // real[i]: phrase i follows phrase i-1 and is not its successor (a block start in
                                    // every occurrence of phrases i-1, i, ...); without a cap, real = !cont but for i = 0
    std::vector<int32_t> bstart;    // bstart[i]: the block of phrase i starts at phrase bstart[i]
    std::vector<uint64_t> ph;       // with shortcuts: prefix fingerprints of pid (ph[t]: pid[0 .. t))
    std::vector<uint32_t> zc;       //   and the number of pid[0 .. t) that are 0
    // the fingerprint of pid[a .. b)
    inline uint64_t fp(const parse_shortcuts &SC, size_t a, size_t b) const {
        return parse_shortcuts::submod(ph[b], parse_shortcuts::mulmod(ph[a], SC.pw[b - a]));
    }
    void finish(const two_level &L) {
        size_t z = S.size();
        if (L.SC) {
            ph.assign(z + 1, 0); zc.assign(z + 1, 0);
            for (size_t t = 0; t < z; ++t) {
                ph[t + 1] = parse_shortcuts::addmod(parse_shortcuts::mulmod(ph[t], parse_shortcuts::BASE), pid[t]);
                zc[t + 1] = zc[t] + (pid[t] == 0);
            }
        }
        cont.assign(z, 0); real.assign(z, 0); bstart.assign(z, 0);
        const uint32_t sw = L.succp.w;
        for (size_t i = 0; i < z; ++i) {
            if (L.has_blocks && i + 16 < z) __builtin_prefetch(&L.succp.d[((uint64_t)pid[i + 16] * sw) >> 6]);   // succ(pid[i + 16]), later
            const bool succ = L.has_blocks && i > 0 && pid[i] && pid[i - 1] && L.succ(pid[i - 1]) == pid[i];
            real[i] = L.has_blocks && i > 0 && pid[i] && pid[i - 1] && !succ;
            // a cap B: the block also ends after B phrases, counted from its run's start (as in the text)
            cont[i] = succ && (!L.maxblock || (uint64_t)(i - bstart[i - 1]) < L.maxblock);
            bstart[i] = cont[i] ? bstart[i - 1] : (int32_t)i;
        }
    }
};

// Parses P (into pi) and rc(P) (into qi) with one scan for syncmers and one dictionary lookup per phrase.
inline void parse_pattern(const two_level &L, const char *Pc, uint64_t len, pattern_parse &pi, pattern_parse &qi) {
    const PhraseDict &D = L.dict;
    const uint8_t *x = reinterpret_cast<const uint8_t *>(Pc);
    const int K = L.K;
    pi.S.clear();
    PROF(u64 prof_t = ps_prof::now());
    for_each_syncmer(x, len, K, L.SM, [&](uint64_t q) { pi.S.push_back((int64_t)q); });
    PROF(g_prof.cyc[11] += ps_prof::now() - prof_t; prof_t = ps_prof::now());
    const size_t z = pi.S.size(), np = z ? z - 1 : 0;
    const int64_t m = (int64_t)len;
    qi.S.resize(z);
    for (size_t i = 0; i < z; ++i) qi.S[i] = m - K - pi.S[z - 1 - i];
    static thread_local std::vector<uint64_t> h;
    h.resize(np);
    for (size_t i = 0; i < np; ++i) h[i] = phrase_hash(x + pi.S[i], pi.S[i + 1] + K - pi.S[i]);
    PROF(g_prof.cyc[12] += ps_prof::now() - prof_t; prof_t = ps_prof::now());
    constexpr size_t AHEAD = 32;                         // bucket starts 2 AHEAD phrases ahead, records AHEAD ahead
    for (size_t i = 0; i < np && i < 2 * AHEAD; ++i) D.prefetch_bucket(h[i]);
    for (size_t i = 0; i < np && i < AHEAD; ++i) D.prefetch(h[i]);
    pi.pid.assign(z, 0); qi.pid.assign(z, 0);
    for (size_t i = 0; i < np; ++i) {
        if (i + 2 * AHEAD < np) D.prefetch_bucket(h[i + 2 * AHEAD]);
        if (i + AHEAD < np) D.prefetch(h[i + AHEAD]);
        uint32_t f, r;
        if (D.lookup(h[i], f, r)) { pi.pid[i] = f; qi.pid[np - 1 - i] = r; }
    }
    PROF(g_prof.cyc[13] += ps_prof::now() - prof_t; prof_t = ps_prof::now());
    pi.finish(L); qi.finish(L);
    PROF(g_prof.cyc[15] += ps_prof::now() - prof_t);
}

struct search_counters {
    u64 searches = 0, char_steps = 0, parse_steps = 0, block_steps = 0, kmer_tests = 0, kmer_restarts = 0;
    u64 stpd_jumps = 0, stpd_compares = 0;     // STPD: binary searches over the sample, and text comparisons
    // KeBaB with --longest (mems.hpp): pseudo-MEMs in the reads, pseudo-MEMs searched (BML run on them),
    // batches, reads that needed a second batch, reads that ran the whole-read fallback
    u64 kb_pseudo = 0, kb_searched = 0, kb_batches = 0, kb_multibatch = 0, kb_fallbacks = 0;
    u64 kb_fb_few = 0, kb_fb_short = 0, kb_seeded = 0;        // --kebab-check fallbacks: at most T pseudo-MEMs; too few long MEMs
    // STPD over the parse: character steps at the right end (backward search), phrase comparisons in the
    // middle (one per pattern phrase), parse-sample binary searches and their comparisons, and character
    // steps of the character-level STPD at the left end
    u64 ps_right = 0, ps_mid = 0, ps_jumps = 0, ps_compares = 0, ps_left = 0;
    // two-level search with blocks: where the parse steps come from.  entry: before the first block attempt (the
    // right end of the search); inside: re-crossing a block phrase by phrase, after a block attempt that failed or
    // backed up, when the match went on past it; end: the same when the match ended there (a failed parse step or
    // the left end of the search).  Block attempts: block steps that found the symbol absent (nosym, not counted
    // as steps) or an empty interval (bfail); back-ups (to the last exact interval before the last block); exact
    // exits (Lemma "backup", first case)
    u64 pe_entry = 0, pe_inside = 0, pe_end = 0, b_nosym = 0, b_fail = 0, b_backups = 0, b_exact = 0, b_attempts = 0;
    // shortcuts over phrases: entrances met (the current phrase has a shortcut), the pattern's phrases not alpha,
    // shortcuts taken, failed midway (alpha matched, alpha Z does not occur), phrases crossed by those taken
    u64 sc_met = 0, sc_mismatch = 0, sc_taken = 0, sc_failed = 0, sc_crossed = 0;
    void add(const search_counters &o) {
        sc_met += o.sc_met; sc_mismatch += o.sc_mismatch; sc_taken += o.sc_taken; sc_failed += o.sc_failed; sc_crossed += o.sc_crossed;
        pe_entry += o.pe_entry; pe_inside += o.pe_inside; pe_end += o.pe_end; b_nosym += o.b_nosym; b_fail += o.b_fail;
        b_backups += o.b_backups; b_exact += o.b_exact; b_attempts += o.b_attempts;
        ps_right += o.ps_right; ps_mid += o.ps_mid; ps_jumps += o.ps_jumps; ps_compares += o.ps_compares; ps_left += o.ps_left;
        searches += o.searches; char_steps += o.char_steps; parse_steps += o.parse_steps; block_steps += o.block_steps;
        kmer_tests += o.kmer_tests; kmer_restarts += o.kmer_restarts; stpd_jumps += o.stpd_jumps; stpd_compares += o.stpd_compares;
        kb_pseudo += o.kb_pseudo; kb_searched += o.kb_searched; kb_batches += o.kb_batches; kb_multibatch += o.kb_multibatch;
        kb_fallbacks += o.kb_fallbacks; kb_fb_few += o.kb_fb_few; kb_fb_short += o.kb_fb_short; kb_seeded += o.kb_seeded;
    }
};

// Locating with two-level search (and blocks): a toehold, SA at the first row of the current interval, kept
// through every kind of step, as the r-index keeps one at the
// last row:
//   character step c on [sp, ep):  if BWT[sp] = c, t - 1; otherwise the first c in the range starts a BWT run,
//     and its sample SA[x] - 1 is stored per character (csamp, in the order of each character's runs);
//   parse step a on parse rows [plo, phi], toehold u (a phrase index):  if BWT_Z[plo] = a, u - 1; otherwise
//     the sample of the first a-run starting in the range (SA_Z at the starts of the parse BWT's runs, psamp,
//     in row order: prow), minus 1;
//   block step: the same over the block BWT, with block indices (bsamp, brow);
//   into the blocks: the first block row x >= plo is plo itself or the start of a parse BWT run (every row
//     of the range before x holds a phrase that continues into the range's first phrase, every block row one
//     that does not), so SA_Z[x] is u or a sample, and its block is found with bstarts;
//   out of the blocks: the first phrase of the block (bstarts); out of the parse: its text start (pstarts).
// Every occurrence then comes from the toehold by phi^-1 (ikeys, ivals: SA at the ends of the BWT's runs).
struct twolevel_locate {
    u64 n = 0, m = 0, nblocks = 0;
    u64 Cr[257];                        // character c's runs are csamp[Cr[c] .. Cr[c+1])
    PackedInts csamp;
    EliasFano ikeys; PackedInts ivals; u64 last_sa = 0;
    EliasFano prow; PackedInts psamp;   // parse BWT run starts (rows) and SA_Z there
    EliasFano brow; PackedInts bsamp;   // block BWT run starts and SA_B there
    EliasFano bstarts;                  // the first phrase of each block
    EliasFano pstarts;                  // the text start of each phrase
    twolevel_locate() { std::fill(Cr, Cr + 257, 0); }

    inline u64 phi_inv(u64 i) const { u64 t = ikeys.count_less(i + 1) - 1; return ivals.get(t) + (i - ikeys.access(t)); }
    // the toehold after a character step c from [sp, ...) with toehold t; nsp: the new sp (C[c] + rank_c(sp))
    inline u64 char_step(const rlbwt &B, unsigned char c, u64 sp, u64 nsp, u64 t) const {
        if (B.at(sp) == c) return t - 1;
        u64 k = nsp - B.C[c], jc = B.cum[c].count_less(k);
        return csamp.get(Cr[c] + jc);
    }
    // the toehold after a step by symbol c of an EFRank E from rows [i1, ...) with toehold u
    static inline u64 rank_step(const EFRank &E, const EliasFano &row, const PackedInts &samp, uint32_t c, u64 i1, u64 u) {
        const u64 base = (u64)c * E.n, b = E.KS.count_less(base), a1 = E.KS.count_less(base + i1 + 1);
        if (a1 > b) {                                           // the last c-run starting at or before i1
            u64 j = a1 - 1, start = E.KS.access(j) - base, f, fn;
            if (j + 1 < E.nruns) E.FP.access2(j, f, fn); else { f = E.FP.access(j); fn = E.n; }
            if (i1 < start + (fn - f)) return u - 1;
        }
        const u64 s = E.KS.access(a1) - base;                  // the first c-run starting after i1
        return samp.get(row.count_less(s + 1) - 1) - 1;
    }
    inline u64 block_of(u64 v) const {                          // the block starting at phrase v (v = m: the end)
        if (v >= m) return nblocks;
        u64 b = bstarts.pred_index(v);
        if (bstarts.access(b) != v) die("internal error: two-level locate (a block row not at a block start)");
        return b;
    }
    // SA_Z at parse row x, given u = SA_Z[plo], plo <= x, x = plo or a parse BWT run start
    inline u64 parse_sa_at(u64 x, u64 plo, u64 u) const {
        if (x == plo) return u;
        u64 q = prow.count_less(x + 1) - 1;
        if (prow.access(q) != x) die("internal error: two-level locate (the first block row is not a run start)");
        return psamp.get(q);
    }
    u64 bytes_char() const { return csamp.bytes() + ikeys.bytes() + ivals.bytes(); }
    u64 bytes_parse() const { return prow.bytes() + psamp.bytes() + pstarts.bytes(); }
    u64 bytes_blocks() const { return brow.bytes() + bsamp.bytes() + bstarts.bytes(); }
    void save(const std::string &f) const {
        FILE *o = open_or_die(f, "wb"); wmagic(o, "KTTLOC01");
        wr(o, n); wr(o, m); wr(o, nblocks); fwrite(Cr, 8, 257, o);
        csamp.save(o); ikeys.save(o); ivals.save(o); wr(o, last_sa);
        prow.save(o); psamp.save(o); brow.save(o); bsamp.save(o); bstarts.save(o); pstarts.save(o);
        fclose(o);
    }
    void load(const std::string &f) {
        FILE *i = open_or_die(f, "rb"); rmagic(i, "KTTLOC01", f);
        rd(i, n); rd(i, m); rd(i, nblocks); if (fread(Cr, 8, 257, i) != 257) die("short read " + f);
        csamp.load(i); ikeys.load(i); ivals.load(i); rd(i, last_sa);
        prow.load(i); psamp.load(i); brow.load(i); bsamp.load(i); bstarts.load(i); pstarts.load(i);
        fclose(i);
    }
};

#ifdef KATKA_LATCOUNT
// Counting build for the latency study (results/chr19/latency/README.md): steps per symbol in pbwt and bbwt, written
// at exit to $KATKA_LATCOUNT_OUT.{pbwt,bbwt}.steps (one uint64 per symbol id).  Not for timing.
struct lat_count {
    std::vector<uint64_t> p, b;
    inline void step(std::vector<uint64_t> &v, uint64_t c) { if (c >= v.size()) v.resize(c + 1, 0); ++v[c]; }
    ~lat_count() {
        const char *o = getenv("KATKA_LATCOUNT_OUT");
        if (!o) return;
        for (int k = 0; k < 2; ++k) {
            std::string f = std::string(o) + (k ? ".bbwt.steps" : ".pbwt.steps");
            const std::vector<uint64_t> &v = k ? b : p;
            FILE *w = fopen(f.c_str(), "wb");
            if (w) { fwrite(v.data(), 8, v.size(), w); fclose(w); }
        }
    }
};
inline lat_count g_lat;
#define LATCOUNT(x) x
#else
#define LATCOUNT(x)
#endif

#ifdef KATKA_DIACOUNT
// Counting build (◇ and ▷ diagnostics; not for timing): for each two-level search that took a
// successful block step, the block interval [o1, o2) after its last successful one, appended as two uint64 to
// $KATKA_DIACOUNT_OUT (one thread only).
struct dia_intervals {
    FILE *f = nullptr;
    dia_intervals() { const char *o = getenv("KATKA_DIACOUNT_OUT"); if (o) f = fopen(o, "wb"); }
    ~dia_intervals() { if (f) fclose(f); }
    void put(uint64_t a, uint64_t b) { if (f) { uint64_t v[2] = {a, b}; fwrite(v, 8, 2, f); } }
};
inline dia_intervals g_dia;
// The right end: beta' = X[S[idx] .. end) is the partial phrase at the right
// end, alpha = pid[idx - 1] the complete phrase to its left.  Each search falls in one case:
//   0 no parse      the search never reaches the parse (no syncmer, or it is left of begin)
//   1 no alpha      alpha is cut by begin or is not a phrase (no identifier)
//   2 no successor  succ(alpha) = 0: alpha has several distinct successors (or none)
//   3 s, beta'      succ(alpha) = s and s starts with beta'
//   4 s, not beta'  succ(alpha) = s and s does not start with beta' (or the character steps fail first)
// "s starts with beta'" is tested by rows: s's first row in the parse's F column, C[s], lies in the parse
// range for beta' (phrase ids are in the phrases' lexicographic order and the phrases are prefix-free).  It needs
// beta''s character steps, so when they fail the search counts as case 4.  For each case: the searches, the
// right-end character steps, how many of them are past the first K (the characters after alpha's last syncmer),
// how often the character steps fail, how often the parse step for alpha succeeds, and in case 2 the distribution
// of alpha's number of distinct successors (outdeg, from the parse BWT at the first search).  Checks: in cases 3
// and 4 the parse step for alpha succeeds exactly when C[s] is in the range (counted as "disagree").
struct dia_right {
    static constexpr int NC = 5, NH = 65;
    const char *name[NC] = {"no parse", "no alpha", "no successor", "s, beta'", "s, not beta'"};
    u64 searches[NC] = {}, chars[NC] = {}, chars_new[NC] = {}, char_fail[NC] = {}, alpha_ok[NC] = {}, disagree = 0;
    u64 att_taken = 0, att_taken_exact = 0;              // pair attempts that took a step, and those left as exact
    u64 deg[NH] = {}, deg_sum = 0;                       // case 2: outdeg(alpha), the last bucket 64 or more
    std::vector<uint64_t> C;                              // C[y]: the first parse row starting with y; C[sigma + 1] = m + 1
    std::vector<uint32_t> outdeg;
    FILE *af = nullptr;                                   // $KATKA_DIACOUNT_ALPHA: (alpha, case) per search with an alpha
    void init(const two_level &L) {
        if (!C.empty()) return;
        if (const char *o = getenv("KATKA_DIACOUNT_ALPHA")) af = fopen(o, "wb");
        const EFRank &W = L.pbwt; const uint64_t N = L.m + 1;
        if (W.n != N || !W.slot.empty()) die("unexpected parse BWT layout");
        C.assign(L.sigma + 2, 0);
        for (uint64_t j = 0; j < W.nruns; ++j) {
            const uint64_t ks = W.KS.access(j), f = W.FP.access(j), fn = j + 1 < W.nruns ? W.FP.access(j + 1) : N;
            C[ks / N + 1] += fn - f;
        }
        for (uint32_t y = 1; y <= L.sigma + 1; ++y) C[y] += C[y - 1];
        outdeg.assign(L.sigma + 1, 0);
        std::vector<uint32_t> last(L.sigma + 1, UINT32_MAX);
        for (uint64_t j = 0; j < W.nruns; ++j) {          // runs by symbol x, then by position: the rows' first phrases y
            const uint64_t ks = W.KS.access(j), x = ks / N, st = ks % N, f = W.FP.access(j), fn = j + 1 < W.nruns ? W.FP.access(j + 1) : N;
            const uint32_t y1 = (uint32_t)(std::upper_bound(C.begin(), C.end(), st) - C.begin() - 1),
                           y2 = (uint32_t)(std::upper_bound(C.begin(), C.end(), st + (fn - f) - 1) - C.begin() - 1);
            for (uint32_t y = y1; y <= y2; ++y) if (last[x] != y && C[y] < C[y + 1]) { last[x] = y; ++outdeg[x]; }
        }
    }
    void put_alpha(uint32_t a, uint32_t c) { if (af) { uint32_t v[2] = {a, c}; fwrite(v, 4, 2, af); } }
    ~dia_right() {
        if (af) fclose(af);
        if (C.empty()) return;
        u64 tot = 0; for (int c = 0; c < NC; ++c) tot += searches[c];
        fprintf(stderr, "right end: %lu searches; disagree=%lu\n", tot, disagree);
        fprintf(stderr, "pair attempts that took a step: attempts_taken=%lu exact=%lu\n", att_taken, att_taken_exact);
        for (int c = 0; c < NC; ++c)
            fprintf(stderr, "right end case %d (%s): searches=%lu chars=%lu chars_past_K=%lu char_fail=%lu alpha_step_ok=%lu\n",
                    c, name[c], searches[c], chars[c], chars_new[c], char_fail[c], alpha_ok[c]);
        fprintf(stderr, "right end case 2 outdeg: sum=%lu", deg_sum);
        for (int k = 0; k < NH; ++k) if (deg[k]) fprintf(stderr, " %d:%lu", k, deg[k]);
        fprintf(stderr, "\n");
    }
};
inline dia_right g_right;
#define DIACOUNT(...) __VA_ARGS__
#else
#define DIACOUNT(...)
#endif

// Lookahead distance d of the parse steps' prefetches (latency study): $KATKA_LA_D, 0 (none) by default.
inline const int64_t g_la_d = getenv("KATKA_LA_D") ? atoll(getenv("KATKA_LA_D")) : 0;

// The longest suffix X[j..end) of X[begin..end) that occurs in S, and its interval: the same result as
// plain backward search (Theorem "twolevel"; with jumps, Lemmas "blockstart" and "backup").  Returns j.
// With TL (and B, the RLBWT), *toehold is set to SA[out.sp] (if out is not empty).
inline u64 search2(const stepper &st, const two_level &L, const pattern_parse &pi, const char *X, int64_t begin, int64_t end,
                   bool jumps, interval &out, search_counters &cnt, const twolevel_locate *TLc = nullptr, const rlbwt *RB = nullptr,
                   u64 *toehold = nullptr) {
    const int K = L.K;
    interval I = st.full();
    int64_t j = end;
    u64 t = TLc ? TLc->n - 1 : 0;                          // SA[0]: the suffix $
    auto char_steps_to = [&](int64_t stop) -> bool {      // false if a step fails
        if (st.mv && !TLc) {                                // the move structure: (run, offset) at both ends
            if (j <= stop) return true;
            const move_index &M = *st.mv;
            move_index::cursor c = M.at(I.sp, I.ep);
            bool ok = true;
            if (M.tk && I.sp == 0 && I.ep == M.n && j - stop >= M.tk && !st.mv_check) {   // the first tk steps from the table
                u64 key = 0; int t = 0;
                for (; t < M.tk; ++t) { const int a = move_index::code2((unsigned char)X[j - M.tk + t]); if (a < 0) break; key = (key << 2) | (u64)a; }
                if (t == M.tk && M.tab[key].rs != UINT64_MAX) { c = M.tab[key]; j -= M.tk; cnt.char_steps += M.tk; }
            }
            while (j > stop) {
                ++cnt.char_steps;
                const unsigned char ch = (unsigned char)X[j - 1];
                if (!M.step(c, ch)) { ok = false; break; }
                if (st.mv_check && st.csa) {
                    u64 a = I.sp, b = I.ep, sp, ep; st.csa->extend(a, b, ch); M.rows(c, sp, ep); I = {a, b};
                    if (a != sp || b != ep) die("internal error: the move structure's step differs from the RLCSA's");
                }
                --j;
            }
            if (st.mv_check && st.csa && !ok) { u64 a = I.sp, b = I.ep; st.csa->extend(a, b, (unsigned char)X[j - 1]); if (a < b) die("internal error: the move structure's step fails, the RLCSA's does not"); }
            M.rows(c, I.sp, I.ep);
            return ok;
        }
        while (j > stop) {
            ++cnt.char_steps;
            const u64 sp0 = I.sp;
            if (!st.step(I, (unsigned char)X[j - 1])) return false;
            if (TLc) t = TLc->char_step(*RB, (unsigned char)X[j - 1], sp0, I.sp, t);
            --j;
        }
        return true;
    };
    ++cnt.searches;
    const auto &S = pi.S;
    int64_t idx = (int64_t)(std::upper_bound(S.begin(), S.end(), end - K) - S.begin()) - 1;
    PROF(const int pd = g_prof.dir; ++g_prof.tl_searches[pd]; u64 prof_t = ps_prof::now(), prof_c = cnt.char_steps);
    if (g_la_d && idx > 0)                                // lookahead: the new window's first phrases, before its character steps
        for (int64_t q = idx - 1, lo = std::max<int64_t>(0, idx - 1 - 2 * g_la_d); q >= lo; --q) if (pi.pid[q]) L.pbwt.pf_first(pi.pid[q]);
    DIACOUNT(g_right.init(L); const u64 dia_c0 = cnt.char_steps;
             const bool dia_parse = idx >= 0 && S[idx] >= begin, dia_alpha = dia_parse && idx > 0 && S[idx - 1] >= begin && pi.pid[idx - 1];
             const uint32_t dia_s = dia_alpha && L.has_blocks ? L.succ(pi.pid[idx - 1]) : 0;
             int dia_case = !dia_parse ? 0 : !dia_alpha ? 1 : !dia_s ? 2 : 4; bool dia_done = false);
    if (idx >= 0 && S[idx] >= begin && char_steps_to(S[idx])) {
        DIACOUNT({
            uint64_t a, b, q0, q1; L.B.rank1_pair(I.sp, I.ep, q0, q1); ++q0;
            if (dia_alpha) {
                L.pbwt.lf2(pi.pid[idx - 1], q0, q1 + 1, a, b);
                if (dia_s) {
                    const bool in = g_right.C[dia_s] >= q0 && g_right.C[dia_s] <= q1;
                    if (in) dia_case = 3;
                    if (in != (a < b)) ++g_right.disagree;
                }
                if (a < b) ++g_right.alpha_ok[dia_case];
                g_right.put_alpha(pi.pid[idx - 1], (uint32_t)dia_case | (a < b ? 8u : 0u));
                if (!dia_s) { const uint32_t d = g_right.outdeg[pi.pid[idx - 1]]; ++g_right.deg[std::min<uint32_t>(d, 64)]; g_right.deg_sum += d; }
            }
            ++g_right.searches[dia_case]; g_right.chars[dia_case] += cnt.char_steps - dia_c0;
            g_right.chars_new[dia_case] += std::max<int64_t>(0, end - S[idx] - K); dia_done = true;
        });
        PROF(g_prof.cyc[16] += ps_prof::now() - prof_t; g_prof.tl_right[pd] += cnt.char_steps - prof_c; ++g_prof.tl_parse[pd]; prof_t = ps_prof::now();
             const u64 prof_p = cnt.parse_steps);
        // parse rows [plo, phi] (inclusive) for X[S[i] .. end)
        uint64_t plo, phi;
        L.B.rank1_pair(I.sp, I.ep, plo, phi); ++plo;
        PROF(g_prof.cyc[17] += ps_prof::now() - prof_t; prof_t = ps_prof::now());
        u64 u = TLc ? TLc->pstarts.pred_index(t) : 0;       // the toehold's phrase (t starts one: it is at a boundary)
        if (TLc && TLc->pstarts.access(u) != t) die("internal error: two-level locate (the toehold is not at a boundary)");
        int64_t i = idx, noblock_above = idx - 1;
        jumps = jumps && L.has_blocks && !L.SC;               // shortcuts replace block steps
        if (L.SC && TLc) die("shortcuts over phrases do not support locating");
        u64 seg = 0; int segkind = 0;                        // parse steps since the last attempt; 0: entry, 1: re-crossing
        // lookahead (latency study): stage 1 (pf_first) issued down to p1, stage 2 (pf_runs) down to p2, for the phrases
        // 2d and d steps ahead in pi.pid; after a jump (blocks, shortcuts) they restart below the new i
        int64_t p1 = g_la_d && i > 0 ? std::max<int64_t>(0, i - 1 - 2 * g_la_d) - 1 : i - 1, p2 = i - 1;   // stage 1 was issued at entry
        auto lookahead = [&]() {
            if (p1 > i - 1) p1 = i - 1;
            if (p2 > i - 1) p2 = i - 1;
            for (const int64_t lo = std::max<int64_t>(0, i - 1 - 2 * g_la_d); p1 >= lo; --p1) if (pi.pid[p1]) L.pbwt.pf_first(pi.pid[p1]);
            for (const int64_t lo = std::max<int64_t>(0, i - 1 - g_la_d); p2 >= lo; --p2) if (pi.pid[p2]) L.pbwt.pf_runs(pi.pid[p2]);
        };
#ifdef KATKA_NS
        rl_counts &nsr = g_ns.rl[g_ns.dir]; ++nsr.searches; u64 ns_toe = plo; bool ns_in1 = false;
#endif
        DIACOUNT(uint64_t dia_a = 0, dia_b = 0);
        while (i > 0 && S[i - 1] >= begin && pi.pid[i - 1]) {
            if (g_la_d) lookahead();
            if (jumps && i <= noblock_above && !pi.cont[i]) {
                (segkind ? cnt.pe_inside : cnt.pe_entry) += seg; seg = 0;    // the match went on past them
                ++cnt.b_attempts;
                uint64_t blo = L.BS.count_less(plo), bhi = L.BS.count_less(phi + 1);
                int64_t cur = i, taken = 0;
                // the last exact interval before the last block step: the attempt's own (scur = i, the parse interval
                // plo..phi) or the block interval at a real boundary (without a cap, every block start of the pattern
                // is one, and this is the interval before the last block)
                uint64_t sblo = 0, sbhi = 0; int64_t scur = i; u64 sbt = 0;
                u64 bt = 0;                                  // the block toehold
                if (TLc && blo < bhi) bt = TLc->block_of(TLc->parse_sa_at(L.BS.access(blo), plo, u));
                while (cur > 0 && blo < bhi) {
                    int32_t t0 = pi.bstart[cur - 1];
                    if (S[t0] < begin || !pi.pid[t0]) break;
                    // the block [t0, cur - 1] and its side: + if the next phrase is larger than the successor; a block
                    // cut by the cap (the next phrase is the successor) has the key (B, +), after all - and before all +
                    const bool capped = L.maxblock && pi.pid[cur] == L.succ(pi.pid[cur - 1]);
                    const bool plus = capped || pi.pid[cur] > L.succ(pi.pid[cur - 1]);
                    uint32_t y = L.sym(pi.pid[t0], capped ? (uint64_t)L.maxblock : (uint64_t)(cur - 1 - t0), plus);
                    if (!y) { ++cnt.b_nosym; break; }
                    uint64_t o1, o2;
                    L.bbwt.lf2(y, blo, bhi, o1, o2);
                    LATCOUNT(g_lat.step(g_lat.b, y));
                    ++cnt.block_steps;
                    if (o1 >= o2) { ++cnt.b_fail; break; }
                    DIACOUNT(dia_a = o1; dia_b = o2);
                    if (cur != i && pi.real[cur]) { sblo = blo; sbhi = bhi; scur = cur; sbt = bt; }
                    if (TLc) bt = twolevel_locate::rank_step(L.bbwt, TLc->brow, TLc->bsamp, y, blo, bt);
                    blo = o1; bhi = o2; cur = t0; ++taken;
                }
                DIACOUNT(if (taken >= 1) { ++g_right.att_taken; if (!L.can_continue(pi.pid[cur])) ++g_right.att_taken_exact; });
                if (taken >= 1 && !L.can_continue(pi.pid[cur])) {     // exact (Lemma "backup", first case)
                    plo = L.BS.access(blo); phi = L.BS.access(bhi - 1);
                    if (TLc) u = TLc->bstarts.access(bt);
                    i = cur;
                    noblock_above = i;
                    ++cnt.b_exact; segkind = 1;
                    continue;
                }
                if (taken >= 1 && scur != i) {    // back up to the last exact interval before the last block
                    plo = L.BS.access(sblo); phi = L.BS.access(sbhi - 1);
                    if (TLc) u = TLc->bstarts.access(sbt);
                    i = scur;
                    ++cnt.b_backups;
                }
                noblock_above = pi.bstart[i - 1];
                segkind = 1;
            }
            if (L.SC && i < idx) {                           // [plo, phi] starts with the complete phrase pid[i]
                const parse_shortcuts &SC = *L.SC;
                const uint64_t q = SC.find(pi.pid[i]);
                if (q != ~0ull) {
                    ++cnt.sc_met;
                    const int64_t k = (int64_t)SC.K.get(q);
                    if (i - k >= 0 && S[i - k] >= begin && pi.zc[i] == pi.zc[i - k] && pi.fp(SC, i - k, i) == SC.H[q]) {
                        const uint64_t a = SC.A.get(q), lo = a + SC.surv(q, plo), hi = a + SC.surv(q, phi + 1);
                        if (lo < hi) { ++cnt.sc_taken; cnt.sc_crossed += k; plo = lo; phi = hi - 1; i -= k; continue; }
                        ++cnt.sc_failed;                     // alpha Z does not occur: on with parse steps
                    } else ++cnt.sc_mismatch;
                }
            }
            uint64_t o1, o2;
            L.pbwt.lf2(pi.pid[i - 1], plo, phi + 1, o1, o2);
            LATCOUNT(g_lat.step(g_lat.p, pi.pid[i - 1]));
            ++cnt.parse_steps; ++seg;
#ifdef KATKA_NS
            g_ns.step(phi - plo + 1, o1 < o2 ? o2 - o1 : 0);
            ++nsr.reads;
            if (phi == plo) { ++nsr.s1_reads; if (!ns_in1) { ns_in1 = true; ++nsr.s1_stretches; } }
            { u64 a, b; L.pbwt.lf2(pi.pid[i - 1], ns_toe, ns_toe + 1, a, b);
              if (a < b) ns_toe = a; else { ++nsr.toe_dep; if (o1 < o2) { ++nsr.toe_dep_ok; ns_toe = o1; } } }
#endif
            if (o1 >= o2) break;
            if (TLc) u = twolevel_locate::rank_step(L.pbwt, TLc->prow, TLc->psamp, pi.pid[i - 1], plo, u);
            plo = o1; phi = o2 - 1; --i;
        }
        (segkind ? cnt.pe_end : cnt.pe_entry) += seg;       // the steps of the last segment: the match ended there
        DIACOUNT(if (dia_b) g_dia.put(dia_a, dia_b));
#ifdef KATKA_NS
        g_ns.end_search();
#endif
        PROF(g_prof.cyc[18] += ps_prof::now() - prof_t; g_prof.tl_parse_steps[pd] += cnt.parse_steps - prof_p; prof_t = ps_prof::now());
        I.sp = L.B.select1(plo - 1); I.ep = L.B.select1(phi - 1) + 1;
        if (TLc) t = TLc->pstarts.access(u);
        j = S[i];
        PROF(g_prof.cyc[19] += ps_prof::now() - prof_t; prof_t = ps_prof::now(); prof_c = cnt.char_steps);
        char_steps_to(begin);
        PROF(g_prof.cyc[20] += ps_prof::now() - prof_t; g_prof.tl_left[pd] += cnt.char_steps - prof_c);
    } else if (idx < 0 || S[idx] < begin) {
        char_steps_to(begin);
        PROF(g_prof.cyc[21] += ps_prof::now() - prof_t; g_prof.tl_charonly[pd] += cnt.char_steps - prof_c);
    }
    PROF(else { g_prof.cyc[16] += ps_prof::now() - prof_t; g_prof.tl_right[pd] += cnt.char_steps - prof_c; })
    DIACOUNT(if (!dia_done) {                             // no parse, or the right end's character steps failed
        ++g_right.searches[dia_case]; g_right.chars[dia_case] += cnt.char_steps - dia_c0; if (dia_parse) ++g_right.char_fail[dia_case]; });
    out = I;
    if (toehold) *toehold = t;
    return (u64)j;
}

}  // namespace katka
