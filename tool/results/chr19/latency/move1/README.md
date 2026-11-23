# Latency on plain search with the move structure, part 1: the bound (2026-11-22)

*Commit hashes in this README refer to the private development history of this code; those commits are not in this repository.*

chr19, the two query haplotypes, `find -L 1000 --format mems --stats -j 1 --move c<N>` (plain search on the move
structure alone), 16 / 32 / 64 individuals, BML and FB, without and with huge pages
(`GLIBC_TUNABLES=glibc.malloc.hugetlb=1`). There are no code changes. Every run's MEMs and intervals equal the paper's
`blocks/q<N>s.blocks.tsv`.

- **perf:** run at `perf_event_paranoid` 2, so every count is user space only.
- **Counters:** as in B (`../README.md`), two groups of 3 events, each beside the fixed cycles and instructions, with no
  multiplexing. Each group runs once on the queries and once on a 100-character query (index load and little else),
  and the search's counts are the difference.
- **Scripts:** `experiments/latency/p1_stat.sh`, `p1_summary.py`, `p1_classify.py`, `p1_slots.py`. The perf data stay in
  `~/data/latency/p1/`.

## 1. Counters (`summary.md`, `stat/`)

Shares of the search's cycles:

| row | huge pages | IPC | stalled, any reason | on any memory access | on an L3 miss | page walks |
|---|---|---:|---:|---:|---:|---:|
| plain, move structure | without | 0.37–0.38 | 76.4–76.8% | 74.2–74.6% | 56.4–56.9% | 17.3–18.2% |
| plain, move structure | with | 0.45–0.46 | 72.3–72.7% | 69.8–70.3% | 64.3–64.6% | 0.1% |
| heavy paths θ = 90%, with the first-run table | without | 1.56–1.60 | 37.2–38.6% | 30.8–32.2% | 19.8–21.5% | 15.1–16.4% |
| heavy paths θ = 90%, with the first-run table | with | 1.69–1.73 | 32.9–34.4% | 26.3–28.0% | 17.9–19.9% | 1.1–1.6% |

The ranges are over 16 / 32 / 64 individuals and BML / FB. Every cell is in `summary.md`.

- **The L3-miss share rises with huge pages on plain search because the cycles fall, not because the stalls grow.**
  At 16 individuals with BML, the stall cycles go from 4.94 to 4.67 × 10^10, and the page walks, about 18% of cycles,
  disappear.
- **L3-miss loads per run:** plain search has 1.35–1.67 × 10^8, and heavy paths with the table 1.1–1.3 × 10^7.

## 2. Bounds on plain search (`summary.md`)

- **Bound L3:** time × (1 − share stalled on an L3 miss).
- **Bound mem:** time × (1 − share stalled on any memory access).
- **The times** are the final timing session's medians (`../final/runs.tsv`).
- **"Best"** is the fastest two-level row in the same mode, algorithm and size. It is heavy paths θ = 80% with the
  first-run table everywhere; θ = 90% with the table is within 0.06 s.

| individuals | algorithm | huge pages | time | bound L3 | bound mem | best | bound L3 / best | bound mem / best |
|---:|---|---|---:|---:|---:|---:|---:|---:|
| 16 | BML | without | 19.25 | 8.38 | 4.97 | 3.10 | 2.70 | 1.60 |
| 16 | BML | with | 15.99 | 5.71 | 4.83 | 2.75 | 2.08 | 1.76 |
| 16 | FB | without | 18.79 | 8.10 | 4.83 | 3.09 | 2.62 | 1.56 |
| 16 | FB | with | 15.62 | 5.55 | 4.70 | 2.73 | 2.03 | 1.72 |
| 32 | BML | without | 21.37 | 9.30 | 5.47 | 3.01 | 3.09 | 1.82 |
| 32 | BML | with | 17.70 | 6.28 | 5.30 | 2.65 | 2.36 | 1.99 |
| 32 | FB | without | 21.07 | 9.18 | 5.39 | 2.95 | 3.11 | 1.83 |
| 32 | FB | with | 17.45 | 6.17 | 5.21 | 2.62 | 2.36 | 1.99 |
| 64 | BML | without | 22.98 | 10.02 | 5.84 | 3.08 | 3.26 | 1.90 |
| 64 | BML | with | 18.98 | 6.74 | 5.67 | 2.69 | 2.50 | 2.11 |
| 64 | FB | without | 22.74 | 9.91 | 5.77 | 2.97 | 3.34 | 1.94 |
| 64 | FB | with | 18.77 | 6.64 | 5.59 | 2.63 | 2.53 | 2.13 |

- **Even if latency hiding removed every cycle stalled on memory,** plain search on the move structure would still be
  1.6–1.9 times slower than the best two-level row without huge pages, and 1.7–2.1 times slower with them. Removing
  only the L3-miss stalls leaves it 2.6–3.3 and 2.0–2.5 times slower.
- **These are optimistic bounds for plain search.** They assume that the stall cycles vanish and that nothing else
  grows: no cost for switching, no extra instructions, no bandwidth limit.

## 3. The best two-level row's stalls

Heavy paths θ = 90% with the first-run table, from the table in section 1:
- 31–32% of its cycles are stalled on memory without huge pages, and 26–28% with them;
- 20–21% and 18–20% are stalled on an L3 miss.

## 4. Where the two-level rows' L3-miss stalls fall (`../b/`)

These come from B's classified PEBS samples of L3-miss loads and B's counters, all without huge pages.
- **The approximation:** the share of cycles stalled on L3 misses is split over the classes in proportion to their
  L3-miss loads.
- **Heavy paths with the table** had no B data, so it took two new B-style runs, at 16 and 64 (`b_perf.sh` with
  `bin/katka-la-g` and no lookahead, which is the table alone; `../b/q<N>.heavy09.t.*`, `../b/b_summary.t.txt`).

| row | individuals | stalled on an L3 miss | in the text's move structure | in the parse's index | in the pattern's parse | other |
|---|---:|---:|---:|---:|---:|---:|
| two-level, no jumps, after C and D (`b_summary.cd.txt`) | 16 | 11.9% | 6.7% † | 2.9% | 1.1% | 1.2% † |
| | 64 | 10.3% | 4.6% † | 3.9% | 0.9% | 0.9% † |
| heavy paths θ = 90%, unchanged (`b_summary.txt`) | 16 | 21.7% | 6.3% † | 5.0% | 3.3% † | 7.1% † |
| heavy paths θ = 90%, with the table (`b_summary.t.txt`) | 16 | 21.5% | 5.8% † | 4.8% | 4.2% † | 6.8% † |
| | 64 | 20.7% | 5.0% † | 4.9% | 4.1% † | 6.7% † |

The shares are of all cycles.
- **Plain two-level:** about 8% becomes 6.7% at 16 individuals and 4.6% at 64.
- **Heavy paths:** about 17% becomes 5–6%.
- **† Corrected (2026-11-23).**
  - **What the table first read:**
    - the text's column: 7.6 / 5.3%, 13.0%, 11.8 / 11.1%;
    - the pattern's parse on heavy paths: 2.9%, 3.8 / 3.8%;
    - "other": 0.3 / 0.2%, 0.7%, 1.1 / 1.0%;
    - the bullets: "about 8% is confirmed (7.6%)" and "about 17% is corrected to 11–13%".
  - **The cause:** B's classifier counted every sample in `search2` outside the parse's index as the text index (see
    `../README.md`, B).
  - **The fix:** the shares now come from `../b/*.class.strict.txt`. "Other" now holds what left the text index: on heavy
    paths, the shortcuts and `search2`'s shortcut test; elsewhere, B's rank and select and `search2` itself. The
    parse's index column is unchanged.

## 5. Plain search's L3-miss loads by site (`pebs/`)

The sample: `perf record -d -e mem_load_retired.l3_miss:upp -c 503` on `bin/katka-g` (the same build with `-g`), at 16
individuals with BML. It took about 275,000 samples in each mode; huge pages on is the main case, and
off is shown for comparison.

| site | with huge pages | without |
|---|---:|---:|
| the LF step's records (`while (o >= len(q))`, move.hpp:101) | 99.46% | 99.44% |
| — of which, fast-forward reads crossing into a new line (at most) | 2.21% | 3.60% |
| — so the destination row (at least) | 97.25% | 95.84% |
| the records at the interval's ends (`sym(a)`, `sym(e)`) | 0.02% | 0.10% |
| repositioning an end when the character does not match (scans, runs_of, the new end's record) | 0.06% | 0.05% |
| the 8-mer table | 0.06% | 0.05% |
| rows ↔ cursors (Elias–Fano: select, count_less, access) | 0.22% | 0.22% |
| everything else | 0.18% | 0.12% |

- **Destination and fast-forward share one load instruction.** `move_index::step` is not inlined in this build, and
  each end's `lf` loop uses one load for the destination row and for the rows after it (`pebs/lf_loop.disasm.txt`:
  0x3376b for the start, 0x337ab for the end). They are told apart by data address (`p1_slots.py`).
  - A record is 8 bytes, so there are 8 to a line.
  - Destination rows fall evenly on the 8 slots of a line.
  - A fast-forward read misses only when it crosses into the next line, at slot 0.
  - The excess at slot 0 over the other slots' mean, 2.2% (3.6% without huge pages), bounds the fast-forward misses.
- **The two ends' loads take 52% and 47% of the misses.** So the start's and the end's destinations usually lie on
  different lines.
- **For part 2:** the yield belongs in `move_index::step`, between reading the two source records, when both
  destinations are known (`q = dest(q)`), and the fast-forward loops. Prefetching both destinations' lines covers
  about 97% of the misses, and prefetching the next line too would cover the remaining 2–4%. Nothing else is worth a
  yield.
