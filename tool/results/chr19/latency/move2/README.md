# Latency on plain search with the move structure, part 2: chopping and interleaving (2026-11-22)

*Commit hashes in this README refer to the private development history of this code; those commits are not in this repository.*

Plain search on the move structure only (`find -L 1000 --format mems --stats -j 1 [--fb] --move c<N>`), chr19, the two
query haplotypes, 16 / 32 / 64 individuals. The code is on branch `latency`, 5020fc6, and builds `bin/katka-chop`. It
is off unless the environment variables below are set.

## The code

- **Chopping** (`src/mems.hpp`: `bml_piece`, `fb_piece`, `chop_filter`; `src/main.cpp`: `chop_chunk`):
  - `KATKA_CHOP=64` cuts every segment (each of the four patterns here) evenly into pieces, 64 in all, shared in
    proportion to length (21 / 11 / 6 / 26).
  - Each piece runs the usual BML or forward–backward loop with its start as the pattern's left end and no right
    boundary.
  - The pieces' MEMs are joined in order with a filter: drop a MEM that starts exactly at a cut when the last
    MEM kept has the same right end. 56–58 tails are dropped with 64 pieces.
  - Each segment gets its own copy of the search state. The joined MEMs are written by the usual output code
    (`finder::run`, through `segment_search::replay`), so the output format is untouched.
  - Test settings: `KATKA_CHOP_LEN=l` (pieces of about l characters) and `KATKA_CHOP_AT=c1,c2,...` (cuts at given
    offsets).
- **Interleaving** (`src/coro.hpp`; `move_index::step<true>` in `src/move.hpp`):
  - `KATKA_CO_K=K` runs the chunk's pieces in K stackful coroutines on the one thread, round-robin, drawing them from
    one queue over all four patterns.
  - The switch is 15 x86-64 instructions, `katka_ctx_swap`: it saves the six callee-saved registers and the stack
    pointer and loads another coroutine's. No system call; `swapcontext` is not used.
  - The yields:
    - `step<true>` reads both ends' source records as before and computes their destinations (`a = dest(a)`,
      `e = dest(e)`);
    - it prefetches the two destination records and yields to the next piece;
    - then it fast-forwards.
    - This is the only yield point, because part 1 found that at least 96–97% of the L3-miss loads are these
      destination records and under 1% are anywhere else.
  - The plain loop (`steps_left`) calls `step<true>` only while a scheduler is running (`g_co`). Every other caller,
    two-level search included, gets `step<false>`, which is the unchanged code.
- **Not restructured:** the search itself is not restructured, beyond the piece loops and the one yield.

## Checks (`checks.log`, `experiments/latency/p2_checks.sh`, `p2_brute.py`): all passed

Every output below is byte-identical to the unchopped search's, MEM for MEM, with BWT intervals.
- **A.** 16 / 32 / 64 individuals, BML and FB: 64 pieces one by one, and interleaved with K = 8, 16 and 32. The
  reference is the unchanged binary's output from the final timing session.
- **B.** Pieces far shorter than typical MEMs: about 1,000 characters, 100 characters, and 999 (one below L), one by
  one and with K = 8.
  - The work grows as M² / 2B, so these ran on the first 2 Mbp of the first pattern, at all three sizes, with BML and
    FB.
  - They used up to 20,000 pieces and dropped up to 18,766 tails.
- **C.** Cuts exactly at 114 MEM starts (every 200th MEM at 16 individuals), one character before them, and one
  after, one by one and with K = 8, BML and FB.
- **D.** Brute force on a small synthetic collection: 9 sequences of 30,000 characters on both strands, L = 40, 80
  MEMs.
  - The brute force computes matching statistics by substring search.
  - It agrees with the unchopped search, and with pieces of 7 and of 50 characters, one by one and with K = 4, for
    BML and FB.

## The cost of a switch (`co_bench.txt`, `experiments/latency/co_bench.cpp`)

| coroutines | ns per yield | TSC cycles per yield |
|---:|---:|---:|
| 1 (no switch) | 2.1 | 6.8 |
| 2 | 11.5 | 36.7 |
| 8 | 13.3 | 42.3 |
| 16 | 13.7 | 43.7 |
| 32 | 13.8 | 43.9 |

- **An empty loop of yields:** a switch takes about 11–14 ns, against about 80–100 ns for an L3 miss. Most of it is
  probably the mispredicted return.
- **Plain search yields once per backward step on the move structure,** 260 million times at 16 individuals. At the
  benchmark's cost that would be up to 3.5 s, but the interleaved runs take 6.3 s in all, so much of the switching
  overlaps with outstanding misses.
- **Instructions** grow by 16%, about 21 more per step (`stat/counters.md`).

## Timing (`time/`; `experiments/latency/final.sh`, `p2_table.py`)

- **Session:** one session on the idle machine, 13:23–14:29, by the final timing session's rules: one thread pinned to
  CPU 0, an 11 GB cgroup, load time excluded, the median of 3 rounds, every run kept, and 2 more runs where the 3
  without huge pages differed by more than 3% (†).
- **Modes:** each row ran without and with huge pages, the two modes interleaved row by row.
- **The rows:**
  - the unchanged row (`bin/katka-base`);
  - chopping alone, and interleaving with K = 8, 16 and 32;
  - K = 4 and 6, as extra rows beyond K = 8, 16 and 32;
  - the best two-level row, heavy paths θ = 80% with the first-run table (`bin/katka-table`), and its unchanged base.
- **Outputs:** every output is byte-identical to the unchanged binary's.

BML / FB in seconds, 16 / 32 / 64 individuals:

| row | without huge pages | with huge pages |
|---|---|---|
| plain, unchanged | 19.20† / 18.80†, 21.36 / 21.06, 23.01 / 22.75 | 15.97 / 15.63, 17.69 / 17.45, 18.96 / 18.80 |
| chopped, one by one | 19.39 / 18.97, 21.58 / 21.26, 23.18 / 22.94 | 16.09 / 15.76, 17.82 / 17.57, 19.13 / 18.92 |
| interleaved, K = 4 (extra) | 8.85 / 8.63, 9.79 / 9.62, 10.56 / 10.44 | 7.10 / 6.98, 7.87 / 7.75, 8.53 / 8.42 |
| interleaved, K = 6 (extra) | 9.16 / 8.91, 10.09 / 9.91, 10.84 / 10.76 | 6.60 / 6.47, 7.07 / 6.99, 7.69 / 7.61 |
| interleaved, K = 8 | 9.39† / 9.15, 10.44 / 10.27, 11.24 / 11.12 | 6.31 / 6.25, 6.99 / 6.94, 7.58 / 7.54 |
| interleaved, K = 16 | 11.03 / 10.81†, 12.35† / 11.94, 13.15 / 12.86 | 7.27 / 7.14, 7.87 / 7.86, 8.62 / 8.82 |
| interleaved, K = 32 | 11.95 / 11.42, 13.21 / 12.88, 14.18 / 13.96† | 8.46 / 8.35, 9.47 / 9.13, 10.05 / 9.96 |
| heavy paths θ = 80%, with the table | 3.11 / 3.09, 3.01 / 2.95, 3.07 / 2.99 | 2.76 / 2.73, 2.67 / 2.62, 2.75 / 2.64 |

- **Chopping alone costs 0.7–1.0%.** It does 0.5% more backward steps (261.0 against 259.8 million at 16 individuals),
  close to study A's estimate of the wasted work.
- **Speedup of the best K over the unchanged row** (`time/tables.md` has every K):
  - with huge pages: K = 8, 2.49–2.53 times;
  - without huge pages: of K = 8, 16 and 32, K = 8, 2.04–2.06 times; with the extra rows, K = 4, 2.17–2.19 times.
- **K = 16 and 32 are slower than 8.** Likely the reason: each step issues two prefetches, so K = 8 already asks for
  16 lines at once, and an i7-8700 core can track only 12 outstanding misses (its line-fill buffers). This was not
  measured.
- **Why the best K is smaller without huge pages:** there, the interleaved runs spend 37–41% of their cycles in page
  walks (`stat/counters.md`). Prefetches hide the misses but not the walks, so more coroutines only add walks.
- **Stability:** 11 of 108 cells spread over 3%.
  - The largest are the session's first runs: the unchanged row at 16, at 20.38 then 19.20–19.24 s.
  - The other is K = 8 without huge pages at 16, whose first run was 8.68 s and the other four 9.37–9.41 s.
- **Huge pages:** 85–92% of resident memory.

**Headline ratios,** plain search over the best two-level row of this session, BML / FB, 16 / 32 / 64:

| plain search | without huge pages | with huge pages |
|---|---|---|
| unchanged | 6.17 / 6.08, 7.10 / 7.13, 7.49 / 7.62 | 5.79 / 5.72, 6.62 / 6.67, 6.90 / 7.11 |
| interleaved, best of K = 8, 16, 32 (K = 8) | 3.02 / 2.96, 3.47 / 3.48, 3.66 / 3.72 | 2.29 / 2.29, 2.62 / 2.65, 2.76 / 2.85 |
| interleaved, best K with the extra rows | 2.84 / 2.79, 3.25 / 3.26, 3.44 / 3.50 (K = 4) | the same as K = 8 |

With huge pages, part 1's bound on these ratios (all memory stalls removed) was 1.76 / 1.99 / 2.11 (BML).

## The stalls that remain (`stat/counters.md`; `experiments/latency/p1_stat.sh`, rows `co8` and `co4`)

These are part 1's counters on K = 8 and K = 4, at 16 individuals: user space only, with the 100-character run
subtracted. Under perf, K = 8 takes 6.71 s with BML and huge pages.

| row (BML, 16) | huge pages | IPC | stalled, any reason | on any memory access | on an L3 miss | L3-miss loads | page walks |
|---|---|---:|---:|---:|---:|---:|---:|
| unchanged | with | 0.46 | 72.3% | 69.8% | 64.3% | 1.43 × 10^8 | 0.1% |
| interleaved, K = 8 | with | 1.34 | 37.6% | 32.1% | 11.6% | 1.01 × 10^7 | 0.2% |
| unchanged | without | 0.38 | 76.4% | 74.2% | 56.5% | 1.45 × 10^8 | 18.2% |
| interleaved, K = 4 | without | 0.97 | 50.5% | 46.5% | 7.5% | 7.1 × 10^6 | 40.6% |
| interleaved, K = 8 | without | 0.92 | 51.9% | 47.8% | 10.0% | 9.4 × 10^6 | 37.5% |

- **With huge pages and K = 8,** 32% of cycles are still stalled on memory and 12% on an L3 miss.
- **Demand L3-miss loads fall 14-fold.** The prefetches now bring those lines in: they are software prefetches, which
  this event does not count.
- **Cycles fall from 7.26 to 2.89 × 10^10.**

## Would the two-level rows get this for free?

Not really.
- **The yield in `move_index::step` would apply unchanged** to two-level search's character steps on the move structure
  (`twolevel.hpp`, the loop that calls `M.step`), and chopping's piece loops call the same `back` / `extend_left` /
  `forward` that two-level search provides.
- **But those character steps carry only part of the two-level rows' stalls:** 7.6% / 5.3% of cycles for plain
  two-level, and 11–12% for heavy paths with the table (part 1, step 4).
- **The rest is in the parse's index and the pattern's parse,** which would need their own prefetch-and-yield points
  in `EFRank::lf2` and in `parse_pattern`.
- **Chopping a two-level search is not checked:** each piece would have to start at a phrase boundary, and the pattern's
  parse would be shared.
- No yield was enabled in the two-level rows.
