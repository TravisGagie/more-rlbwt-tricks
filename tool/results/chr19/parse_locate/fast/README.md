# Locating from the parse alone, faster walks, timed as Table tab:textless (2026-11-26)

Code: branch `textless`, 3e48f1a (`bin/katka-ploc2` is `src/main.cpp` at that commit). Earlier version: 8912148
(`bin/katka-ploc0`), documented in `../regular/README.md`.

## What changed

1. **The walks of a segment are batched.** The search keeps each reported MEM's rows, and walks them all at the end of
   the segment. The queries are 4 reads, so each batch has tens of thousands of rows. `--locate-out` is written after
   the walks.
2. **Staged interleaving** (`parse_regular_sample::locate_staged`). Each Ψ step is split into five stages, and each
   stage reads one level of the Elias–Fano arrays and prefetches the next:
   - FP's and the mark's select-zero block;
   - their bucket;
   - the sample test and FP's predecessor;
   - KS's fine sample;
   - KS's entry.

   12 lanes move through the stages together. Plain lockstep interleaving, without the stages, gained only about 25%
   and stopped improving at 2 lanes (`bin/walk-bench`): one step is a chain of about a dozen dependent loads, so one
   lane already fills the reorder buffer.
3. **A run of rows at a time** (`locate_staged_runs`).
   - Rows that are consecutive and in one run of F move to consecutive rows by Ψ, through the same phrase. A MEM's
     rows usually are, so they walk as one piece.
   - A piece splits where it crosses the end of a run. When it holds a sampled row, that row is done and the piece
     shrinks to the rows not yet done.
   - The steps per position fall from 31.5 to 14.1 / 13.9 / 12.4 at 16 / 32 / 64 individuals (piece steps).
4. **FP's predecessor by one select-zero** (`EliasFano::pred`), instead of `count_less` and `access`. The division by N
   is by a reciprocal.
5. **The left end is swapped, not copied.** This is the record of where the forward search leaves the parse.

**Walking in both directions** (backward by LF over the run heads by row, which the `.tl2` already has) was not built.
- Alternating forward and backward steps from one row reaches a sample after about 2·min(d, 64 − d) steps, 32 on
  average: no fewer steps than forward alone (31.5).
- Its gain would only be the second dependent chain, which interleaving already provides.
- A backward step reads more than a forward one: the run head's predecessor and symbol, KS's select-zero, FP, and the
  phrase length.

## Checks (`checks.txt`)

For every configuration timed, BML and FB, at 16 / 32 / 64 individuals, L = 1000:
- every position was checked against the text (`$KATKA_PLOC_CHECK`: holds rc(MEM), distinct, as many as occ);
- the `--locate-out` file is byte-identical to 8912148's checked run (`~/diamond/ploc/ref8912148/`);
- the MEMs and occurrences equal the reference's.

The configurations:
- the rows tl, tl+table+la2, tlur+table+la2 and tlh08+table, with s = 64;
- tl with s = 32 and s = 16.

The samples at s = 32 and s = 16 were checked when built: 0 wrong from 2,000,000 random rows each
(`build.s*.txt`).

## Timing (`time/`, `experiments/parse_locate/fast_time.sh`, tables by `fast_table.py` in `time/tables.md`)

**How it was timed:** the rules of the paper's tables.
- One thread pinned to CPU 0, an 11 GB cgroup without swap.
- Huge pages, `GLIBC_TUNABLES=glibc.malloc.hugetlb=1`, as in ce3117d's tables. No system setting was changed.
- Loading excluded; the median of 3; the configurations' order reverses each round.
- The same binary with and without locating.
- Every output is identical to the row's first.
- Largest spread of any configuration: 4.6%.

**Without locating, this session reproduces the paper's** to within 1.6% for every row, size and algorithm
(`time/tables.md`, first table).

**Added time, s = 64 (s), BML / FB:**

| row (Table tab:textless) | 16 | 32 | 64 |
|---|---|---|---|
| two-level | 0.19 / 0.19 | 0.18 / 0.09 | 0.18 / 0.16 |
| with the table and lookahead | 0.13 / 0.19 | 0.11 / 0.13 | 0.18 / 0.12 |
| ▷, jump, tables and lookahead | 0.13 / 0.14 | 0.11 / 0.13 | 0.13 / 0.14 |
| heavy paths θ = 80%, with the table | 0.14 / 0.16 | 0.10 / 0.14 | 0.13 / 0.14 |

With and without (s), BML / FB:

| row | 16 | 32 | 64 |
|---|---|---|---|
| two-level | 5.04 → 5.23 / 5.55 → 5.75 | 5.47 → 5.65 / 6.02 → 6.10 | 6.14 → 6.32 / 6.64 → 6.80 |
| ▷, jump, tables and lookahead | 2.51 → 2.64 / 2.67 → 2.80 | 2.68 → 2.79 / 2.86 → 2.98 | 2.93 → 3.06 / 3.14 → 3.28 |
| heavy paths θ = 80%, with the table | 2.26 → 2.40 / 2.37 → 2.53 | 2.28 → 2.38 / 2.38 → 2.53 | 2.32 → 2.46 / 2.46 → 2.60 |

**The steps, on the row two-level** (added time against without locating, BML / FB, s):

| version | 16 | 32 | 64 | walks alone (s, 16 / 32 / 64) |
|---|---|---|---|---|
| 8912148: one row at a time, after each MEM (`bin/katka-ploc0`) | 0.41 / 0.41 | 0.36 / 0.35 | 0.34 / 0.34 | 0.37 / 0.31 / 0.33 |
| batched per segment, one row at a time | 0.37 / 0.41 | 0.33 / 0.32 | 0.38 / 0.36 | 0.35 / 0.31 / 0.34 |
| 12 staged lanes, a row at a time | 0.26 / 0.21 | 0.23 / 0.18 | 0.27 / 0.28 | 0.21 / 0.19 / 0.22 |
| **12 staged lanes, a run of rows at a time (kept)** | **0.19 / 0.19** | **0.18 / 0.09** | **0.18 / 0.16** | **0.13 / 0.12 / 0.13** |

- **The walks alone** are from find's "walking" timer: 2.8 times faster than 8912148's, at every size.
- **Batching alone** gains nothing.
- **Lanes:** 12 were kept. In `bin/walk-bench`, 8 to 24 lanes are within 3%, and 4 lanes are 10% slower.

**The split of the final version (s = 64), every row:**
- **Collecting the rows:** 0.002–0.004 s.
- **The walks:** 0.128 / 0.116 / 0.124 s at 16 / 32 / 64, the same on every row.
- **The rest:**
  - It is 0–0.06 s, and smaller than the run-to-run spread. On the two fastest rows it is 0.00–0.03 s.
  - The configuration that keeps the rows without walking them (`nowalk`) adds −0.01 to +0.07 s.
  - So recording the left end at every forward search costs nothing measurable any more. It was already cheap: the
    left end is recorded only by forward searches, and now it is swapped, not copied.
  - The earlier 0.07–0.08 s was measured without huge pages.

**The sampling rate (the row two-level; heavy paths in `time/tables.md`):**

| s | sample (MB), 16 / 32 / 64 | walks (s) | added time (s), BML / FB, 16 / 32 / 64 |
|---|---|---|---|
| 64 | 17.9 / 36.1 / 73.5 | 0.13 / 0.12 / 0.13 | 0.19 / 0.19, 0.18 / 0.09, 0.18 / 0.16 |
| 32 | 35.0 / 70.6 / 143.6 | 0.067 / 0.061 / 0.063 | 0.07 / 0.11, 0.05 / 0.02, 0.06 / 0.10 |
| 16 | 68.3 / 137.8 / 280.4 | 0.038 / 0.034 / 0.034 | 0.07 / 0.06, 0.05 / 0.06, 0.05 / 0.07 |

Below s = 32 the added time is within the spread. Each halving of s halves the walks and doubles the sample.

**The index with the sample (MB, as built, 16 / 32 / 64):** the paper's sizes plus 17.9 / 36.1 / 73.5 at s = 64.

| row | the paper's | with the sample |
|---|---|---|
| two-level | 313 / 369 / 470 | 331 / 405 / 543 |
| with the table and lookahead | 381 / 446 / 558 | 399 / 482 / 631 |
| ▷, jump, tables and lookahead | 436 / 519 / 662 | 454 / 555 / 735 |
| heavy paths θ = 80%, with the table | 530 / 618 / 770 | 548 / 654 / 843 |

**Peak resident size with locating at 64 individuals** (s = 64, largest of the rounds, BML and FB):

| row | MB |
|---|---|
| two-level | 907 (826 without) |
| with the table and lookahead | 991 (910) |
| ▷, jump, tables and lookahead | 1,091 (1,009) |
| heavy paths θ = 80%, with the table | 1,274 (1,165) |

## Two-level `--locate` (`tloc/`, `experiments/parse_locate/fast_tloc.sh`)

The setup is `--csa --twolevel` with and without `--locate`, `c<N>.tloc`, under the same rules (huge pages), at 16 /
32 individuals. There is no `c129.tloc`.

| individuals | BML: without → with, added | FB: without → with, added | peak with (MB) |
|---|---|---|---|
| 16 | 3.50 → 4.81: 1.31 | 3.51 → 4.83: 1.32 | 2,259 |
| 32 | 3.61 → 4.91: 1.30 | 3.52 → 4.69: 1.16 | 3,214 |

The spreads at 16 are up to 10.5%, both with and without.

The three figures for two-level `--locate`:
- **1.88 / 1.74 s** come from different runs:
  - The paper's Table 2 has, at 16 individuals, 7.50 s with `--locate` and 5.62 s without.
  - At 32, 1.74 s is 7.05 s minus 5.31 s. The 7.05 s is from `chr19/locpos.log`, a single run of 2026-11-04 that
    also wrote `--locate-out`; I couldn't find which run the 5.31 s came from.
  - These are the wall times of the runs, after loading. They were single runs, not pinned to CPU 0, without huge
    pages, and the two runs of each pair were not alternated.
- **1.46 / 1.36 s:** the same binary, the same session, median of 3, alternating, without huge pages
  (`../regular/time/`).
- **1.31 / 1.30 s (BML):** as before, with huge pages.

The paper's table rows use huge pages, so the comparable figure is the last.
