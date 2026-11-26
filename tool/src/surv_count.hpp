// Survivors of the parse (2026-11-25, "locating from the parse alone").
// Compiled in only with -DKATKA_SURV (experiments/parse_locate/build.sh builds bin/katka-surv); bin/katka is unchanged.
// For every reported MEM, one more two-level search of it (as find --locate runs one), without locating, records:
//   kind     0: X[begin..end) has no phrase boundary, or its right end's character steps fail (no parse level);
//            1: the parse level is entered but no parse or block step is taken (only the right end);
//            2: at least one parse or block step;
//   plen     the pattern's phrases crossed at the parse level (by parse steps, block steps or shortcuts);
//   blocks   the block steps of the search (some may be undone by the backup);
//   surv     the size of the parse interval [plo, phi] when the search leaves the parse (rows, contiguous);
//   brows    how many of those rows start blocks (BS);
//   left     the number of the pattern's phrase starts in [begin, S[i]) at the exit (0: the piece left of the
//            first full phrase lies inside one phrase of every occurrence), and why the parse level stopped
//            (0: S[i-1] < begin or i = 0, 1: pid[i-1] = 0, the phrase is not a phrase of the text);
//   occ      ep - sp at the end of the search.
// At exit, the forward search's parse interval is compared with the MEM's own (stderr, "surv forward check").
// Columns: a, e (the MEM in the read), kind, occ, surv, brows, plen, left, why, left_chars (S[i] - begin), blocks.
// One line per MEM to the file named by KATKA_SURV_OUT (tab-separated).  Single-threaded.
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>
namespace katka {
struct surv_rec {
    bool on = false;
    int kind = 0, why = 0;
    uint64_t surv = 0, brows = 0, left = 0, plen = 0, left_chars = 0, plo = 0;
    void reset() { kind = 0; why = 0; surv = brows = left = plen = left_chars = plo = 0; }
};
inline surv_rec g_surv;
// The same record from the forward search that found the MEM (BML's and forward-backward's Q.forward), kept until
// rc_interval compares it with the MEM's own search: do both leave the parse at the same interval?
inline surv_rec g_surv_fw;
inline bool g_surv_fw_valid = false;
inline uint64_t g_surv_fw_a = 0;   // its start in the read
struct surv_fw_counts {
    uint64_t mems = 0, same = 0, differ = 0, no_parse = 0, other = 0;
    ~surv_fw_counts() { if (mems || other) std::fprintf(stderr, "surv forward check: %lu MEMs, the same parse interval %lu, different %lu, "
                                               "(of them, with no parse level %lu); MEMs not found by a forward search from their start %lu\n", mems, same, differ, no_parse, other); }
};
inline surv_fw_counts g_surv_fwc;
inline FILE *g_surv_out = [] { const char *f = std::getenv("KATKA_SURV_OUT"); return f ? std::fopen(f, "w") : nullptr; }();
}  // namespace katka
