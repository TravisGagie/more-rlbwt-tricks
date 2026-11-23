# Latency in the parse's index of two-level search, and huge pages (2026-11-21/22)

*Commit hashes in this README refer to the private development history of this code; those commits are not in this repository.*

chr19, the two query haplotypes (four patterns, 122,434,624 characters), `find -L 1000 --format mems --move --stats
-j 1`, 16 / 32 / 64 individuals. Times follow the paper's rules: one thread pinned to CPU 0, an 11 GB cgroup without
swap, interleaved rounds, the median of 3, and find's own `wall` time, which excludes loading the index. Every timed or
profiled output file is byte-identical to the unchanged binary's (`bin/katka-base`, the build of `tool` at 57596d5) for
the same row, size and algorithm, and those reference outputs equal the paper's MEMs and intervals
(`blocks/q<N>s.blocks.tsv`, columns 1–5).

Scripts are in `experiments/latency/`. Large files (perf data, the reference outputs) stay in `~/data/latency/`.

## Summary

| row (BML, s; 16 / 32 / 64) | current | first-run table (C) | table + lookahead, d = 2 (D) | lookahead alone, d = 4 |
|---|---|---|---|---|
| two-level, no jumps; move structure | 6.06 / 6.40 / 7.11 | 5.45 / 5.75 / 6.37 | 4.32 / 4.48 / 4.83 | 5.73 / 6.06 / 6.67 |
| two-level, ▷; move structure | 3.66 / 3.65 / 3.92 | 3.49 / 3.50 / 3.75 | 3.33 / 3.42 / 3.68 | |
| two-level, heavy paths θ = 0.9; move structure | 3.24 / 3.15 / 3.22 | 3.13 / 3.06 / 3.11 | | |

- **Sources:** `time/d.table.md` (current, table; heavy paths from `time/c.table.md`), `time/d2.table.md` (d = 2) and
  `time/alone.table.md` (lookahead alone; its same-day current was 5.98 / 6.40 / 7.13). FB times are in the same
  tables.
- **The plain row:** 29–32% faster with C and D. The table gives about 10%, and the lookahead about 20% on top of it.
  Without the table, the lookahead gives only 4–6%.
- **The ▷ row:** 6–9% faster.
- **The distance hardly matters:** d = 1, 2 and 4 agree within 0.02–0.07 s (`time/d1`, `time/d2`, `time/d`), and d = 8
  and 16 are a little slower. With d = 0, the lookahead build's times equal the table's (`time/d.table.md`, `la0`), so
  the code costs nothing when it is off.
- **E (chopping and interleaving) was not done:** after C and D, the interval-dependent L3-miss stalls are 0.8–2.0% of
  cycles on both rows, under the 10% threshold.
- **Huge pages** (`GLIBC_TUNABLES=glibc.malloc.hugetlb=1`, no code change, no root) speed up every row by 4–17% and cut
  the page walks from 13–18% of cycles to 3–4% on the two-level rows (see "Huge pages").

## The reproduced rows (`repro/`)

One round of `experiments/twolevel_ac.sh` (the paper's script) with this build, BML:

| row | 16 | 32 | 64 | paper |
|---|---:|---:|---:|---|
| two-level, no jumps | 6.61 | 6.92 | 7.71 | 6.56 / 6.96 / 7.75 |
| two-level, ▷ | 3.88 | 3.89 | 4.15 | 3.91 / 3.94 / 4.22 |
| heavy paths θ = 0.9 | 3.40 | 3.34 | 3.39 | 3.41 / 3.34 / 3.34 |
| plain move structure | 20.28 | 22.55 | 24.38 | 20.26 / 22.65 / 23.68 |

**Later same-day "current" runs were 6–8% faster** (6.06 against 6.56 on the plain row at 16). Without huge pages, the
plain row's times are bimodal from run to run: about 6.1 s or about 6.5 s at 16 for the unchanged binary, and about
5.45 or 5.9 s with the table (`time/hp1`, runs of `nojumps current` and `table` at q33). The paper's numbers match the
slow mode. With huge pages, the same runs agree within 0.02 s. So these comparisons are always against same-day
"current" runs from the same script.

## Notes on the code (before this study)

- **The search calls `EFRank::lf2`, not `rank2`,** for every step in `pbwt` (and in `bbwt` for the ▷ row).
  - Before this study, `lf2` found c's first run with `KS.count_less(c·n)`, checked that c occurs with `KS.access(b)`,
    and read `FP.access(b)` only when an end's predecessor search landed on c's first run.
  - It never used the `first` / `ffirst` table, whatever sigma is.
  - `EFRank::prepare()` is called only for the RLCSA, never for `pbwt` or `bbwt`.
- **Select-zero** (where `count_less` starts) is a sample every 256 zeros, then a binary search over the per-512-bit
  counts of ones between two samples, then a scan of up to 8 words.
- **`EliasFano::count_less`, `select_`, `access` and `access2` are not inlined;** `lf2` and `lf_one` are inlined into
  `search2`. So samples were attributed through LBR call stacks (see B), not with a noinline build.
- **One parse of the pattern serves both directions (A.4):** `parse_pattern` fills `pi.pid` (for P) and `qi.pid` (for
  rc(P)) from one dictionary lookup per phrase, which returns both ids, before the search starts.

## A. Counts (`a/`; `a_counts.sh`, `lat_counts.cpp`, `a_hist.py`, `a3_mems.py`)

- **The index:** `pbwt` has 8.5 / 9.5 / 11.0 M phrases and 10.3 / 13.0 / 17.4 M runs, about 1.2–1.6 runs per phrase.
  `KS` has 27–29 low bits per run, `FP` 4–5.
- **The steps:** 15.1 / 16.4 / 17.3 M parse steps on the plain row; on the ▷ row, 1.6 / 0.9 / 0.6 M parse steps plus
  2.5 / 3.0 / 3.7 M block steps (`bbwt`; counting build `-DKATKA_LATCOUNT`).
- **Runs per symbol,** share of symbols / share of steps (16 individuals; all sizes in `a/a_hist.txt`):

  | runs | 1 | 2 | 3–4 | 5–8 | 9–16 | 17–64 | 65–256 | > 256 |
  |---|---|---|---|---|---|---|---|---|
  | `pbwt`, symbols | 93.1% | 4.5% | 1.7% | 0.5% | 0.1% | 0.1% | 0.0% | 0.0% |
  | `pbwt`, plain row's steps | 85.2% | 3.9% | 1.9% | 1.2% | 1.0% | 1.7% | 1.1% | 3.9% |
  | `bbwt`, symbols | 63.5% | 22.0% | 10.3% | 2.9% | 0.7% | 0.5% | 0.1% | 0.0% |
  | `bbwt`, ▷ row's steps | 27.5% | 14.7% | 8.8% | 5.5% | 4.8% | 9.0% | 6.0% | 23.8% |

  In bytes (the low bits of `KS` and `FP` for a symbol's runs), 97.6% of `pbwt`'s phrases and 89.1% of the plain row's
  steps take at most 8 bytes.
- **Steps whose symbol's `KS` low bits fit in one 64-byte line / in two:**

  | index | 16 | 32 | 64 |
  |---|---|---|---|
  | plain row, `pbwt` | 87.2 / 94.0% | 87.2 / 93.9% | 85.6 / 93.4% |
  | ▷ row, `pbwt` | 90.5 / 96.8% | 87.7 / 94.4% | 82.8 / 91.5% |
  | ▷ row, `bbwt` | 53.9 / 66.1% | 57.2 / 68.8% | 57.8 / 71.1% |

  In one line of each of `KS.low`, `KS.high`, `FP.low` and `FP.high`: 86.9 / 86.1 / 84.4% of the plain row's steps.
- **MEMs** (the paper's BML output):
  - 22,711 / 18,479 / 16,806 MEMs;
  - longest 136,795 / 136,795 / 176,757;
  - the ten longest at each size are in `a/a3_mems.txt`;
  - the sum of M²/(2B), for B = 122.4 M / p:

    | p | 16 | 32 | 64 |
    |---:|---|---|---|
    | 64 | 0.73% | 1.03% | 1.27% |
    | 128 | 1.45% | 2.06% | 2.54% |
    | 256 | 2.91% | 4.11% | 5.08% |

## B. Where the time and the misses go (`b/`; `b_perf.sh`, `b_classify.py`, `b_summary.py`)

- **Builds:** `bin/katka-g` is the same build with `-g`; its `.text` is byte-identical to `bin/katka`'s, so the profiles
  apply to the timed binary exactly. For B after C and D: `bin/katka-la-g`, whose `.text` equals the lookahead
  build's.
- **Counters:** `perf stat` with two groups of 3 events beside the fixed cycles and instructions, so no multiplexing.
  The search's counts are the full run's minus those of a run on a 100-character query, which loads the index and does
  little else. `perf_event_paranoid` was 2, so all counts are user space only.
- **Profiles:** `perf record -e cycles:u` and `-e mem_load_retired.l3_miss:upp`. PEBS samples the missing load itself.
  - Both are recorded with LBR call stacks (`--call-graph lbr --user-callchains`), the only call graphs allowed at
    setting 2.
  - Every frame is expanded into its inline chain with `addr2line -i`.
  - Samples in the parse's index are classified by the text of the source lines of `lf2` / `lf_one` that inline them,
    read from a copy of the sources taken at build time (`~/data/latency/b/src*`).
- **The classes:**
  - (a) finding c's first run: `count_less(c·n)` and `KS.access(b)`; after C, the table entry;
  - (b) other loads that depend on c only (empty: all of them are in (a));
  - (c1) the predecessor searches for c·n + sp and c·n + ep;
  - (c2) what follows them: `KS.access(j)`, `FP.access2(j)` and the arithmetic of `lf_one`.

**Before C and D** (`b/b_summary.txt`; shares of the search's cycles):

| | plain 16 / 64 | ▷ 16 / 64 | heavy paths 16 | plain move structure 16 |
|---|---|---|---|---|
| stalled on an L3 miss | 23.6 / 25.0% | 21.8 / 22.0% | 21.7% | 56.6% |
| stalled on any memory access | 38.0 / 40.2% | 34.4 / 35.4% | 32.8% | 74.1% |
| page walks | 12.8 / 13.4% | 13.2 / 14.2% | 14.6% | 17.8% |
| instructions per cycle | 1.22 / 1.12 | 1.44 / 1.40 | 1.52 | 0.38 |
| cycle samples in the parse's index | 62.2 / 65.8% | 29.0 / 31.6% (pbwt 12.1 / 5.7, bbwt 17.0 / 25.9) | 18.5% | 0 |
| cycle samples in the text index † | 8.8 / 5.8% | 12.7 / 9.1% | 14.4% | 93.7% |
| cycle samples in pattern parsing | 13.0 / 14.4% | 27.0 / 25.9% | 30.1% † | 0 |
| L3-miss loads in the parse's index | 72.2 / 80.6% | 36.8 / 39.7% (pbwt 24.0 / 14.4, bbwt 12.8 / 25.3) | 23.1% | 0 |
| L3-miss loads in the text index † | 18.6 / 12.5% | 28.6 / 23.3% | 28.9% | 99.6% |
| **X** | **17.0 / 20.1%** | **8.0 / 8.7%** | **5.0%** | **0** |
| X (a) | 10.2 / 10.9% | 5.5 / 5.2% | 3.2% | |
| X (b) | 0 | 0 | 0 | |
| X (c1) | 0.08 / 0.29% | 0.30 / 0.67% | 0.18% | |
| X (c2) | 6.8 / 8.9% | 2.2 / 2.9% | 1.6% | |

- **X** is the share of cycles stalled on an L3 miss times the share of L3-miss loads in the parse's index.
- **† Corrected (2026-11-23).** In the two-level columns, the text index rows first read 11.7 / 7.7%, 23.5 / 21.1% and
  25.6% (cycle samples) and 21.1 / 14.2%, 49.4 / 47.2% and 60.0% (L3-miss loads); heavy paths' pattern parsing first
  read 29.9%.
  - **The bug:** `b_classify.py` matched `stepper` anywhere in a frame's demangled name, and `search2`'s own name
    contains it (`katka::search2(katka::stepper const&, ...)`). So every sample in `search2` outside `EFRank::lf2` was
    counted as the text index: the pair symbols, B's rank and select, `BlockStarts`, the parse shortcuts and the loop
    itself.
  - **The fix:** `B_STRICT=1` matches function names only (`b/*.class.strict.txt`, from the same perf data and binaries,
    whose build-ids match the ones the data record). It counts `search2`'s `char_steps_to` lambda (`{lambda(long)#1}`,
    the first of that signature in each source copy) as character steps, and gives the lookahead's prefetches a class
    of their own.
  - **What is unaffected:** the parse's index rows, X and its parts, and the plain move structure. The default mode
    still reproduces `b/*.class.txt`.
- **(c1) is small** because (a)'s search has just brought c's lines into cache.
- **The computational share:** 63–64% of the parse's index cycle samples in the plain row (51–67% in the ▷ row, 63% in
  heavy paths) are on source lines with under 1% of the index's L3-miss loads: scanning high bits, select, decoding.
  This is approximate, since cycle samples skid. A coarser bound agrees: in the plain row at 16, cycles in the parse's
  index not stalled on an L3 miss are 45% of all cycles, 73% of the index's.
- **One line per row:**
  - plain two-level: mixed, with the parse's index the largest waiter;
  - ▷: mixed, spread over the text index, pattern parsing and both parse indexes;
  - heavy paths: mixed, mostly computation, spread over pattern parsing, the parse's index and the text index;
  - plain move structure: mostly waiting on memory.

**After C and D (d = 2)**, the two target rows (`b/b_summary.cd.txt`):

| | plain 16: before / after | plain 64: before / after | ▷ 16: before / after | ▷ 64: before / after |
|---|---|---|---|---|
| stalled on an L3 miss | 23.6 / 11.9% | 25.0 / 10.3% | 21.8 / 18.6% | 22.0 / 21.0% |
| stalled on any memory access | 38.0 / 23.0% | 40.2 / 21.3% | 34.4 / 30.7% | 35.4 / 33.8% |
| page walks | 12.8 / 18.5% | 13.4 / 20.0% | 13.2 / 15.7% | 14.2 / 16.8% |
| instructions per cycle | 1.22 / 1.71 | 1.12 / 1.67 | 1.44 / 1.57 | 1.40 / 1.46 |
| L3-miss loads in the parse's index | 72.2 / 24.3% | 80.6 / 37.5% | 36.8 / 21.4% | 39.7 / 32.5% |
| **X** | **17.0 / 2.9%** | **20.1 / 3.9%** | **8.0 / 4.0%** | **8.7 / 6.8%** |
| (a) | 10.2 / 2.1% | 10.9 / 2.4% | 5.5 / 3.1% | 5.2 / 4.8% |
| (c1) | 0.08 / 0.01% | 0.29 / 0.04% | 0.30 / 0.04% | 0.67 / 0.13% |
| (c2) | 6.8 / 0.8% | 8.9 / 1.4% | 2.2 / 0.9% | 2.9 / 1.9% |
| **(c) = (c1) + (c2)** | **6.9 / 0.8%** | **9.2 / 1.5%** | **2.5 / 0.9%** | **3.6 / 2.0%** |

- **After C and D, (a) is the table entry's own miss.** In the ▷ row it is mostly `bbwt`'s (2.7 / 4.4% of cycles),
  which the lookahead does not cover.
- **Page walks rose** because the table (68–88 MB) is read at random on 4 KB pages.

## C. The first-run table (`src/bits.hpp`, `EFRank::ft`, `build_first`, `lf2`)

- **The change:** `EFRank` gets a table `ft` with, for each symbol c = 0 .. sigma, c's first run in `KS` (= `count_less(c·n)`)
  and that run's position in F, two 32-bit values (8 bytes per symbol). It is built at load for `pbwt` and `bbwt` by one
  sequential pass over `KS` and `FP`, and is not stored in the index files.
  - `lf2` reads `ft[c]` (and `ft[c + 1]`, usually on the same line, for whether c occurs) instead of running
    `count_less(c·n)` and `KS.access(b)`. `lf_one` uses the stored position instead of `FP.access(b)`.
  - The predecessor searches for c·n + sp and c·n + ep, and `KS.access(j)` / `FP.access2(j)`, are unchanged.
  - The table is empty, and `lf2` works as before, if `nruns` or n reaches 2^32, or with `KATKA_NO_FIRST_RUN=1`.
- **Space:** 68.1 / 76.2 / 88.1 MB for `pbwt`, plus 10.7 / 14.8 / 20.8 MB for `bbwt`, in memory only. Loading takes
  0.2–0.5 s longer (for example 0.7 → 1.0 s, plain, 16).
- **Times:** see the summary. The heavy-path row has no new code, but gets the fix because it steps in `pbwt`.

## D. Lookahead prefetching (`src/bits.hpp`, `pf_first`, `pf_runs`; `src/twolevel.hpp`, `search2`)

- **What is prefetched:** at each parse step, for the phrases 2d and d ahead in `pi.pid` (the direction of travel, in
  both directions, since `search2` gets `pi` or `qi`).
  - **2d ahead:** the phrase's table entry `ft[c]`.
  - **d ahead,** from the entry (j = c's first run):
    - in `KS`: the select-zero sample for (c·n) >> l and the count of ones, high-bit block and words around bit
      (c·n >> l) + j;
    - the low bits of `KS[j]` and its select-one sample;
    - in `FP`: the high-bit word, low bits and select-one sample of `FP[j]`.
  - **At a new BML window:** the entries of its first 2d phrases, before the window's character steps.
  - **After a jump** (blocks, shortcuts): the stages restart below the new position.
- **Without the table** (`KATKA_NO_FIRST_RUN=1`): the select-zero sample at 2d, and at d the counts and the high-bit
  block it points to.
- **Not covered:** `bbwt`. The ▷ row's block symbols are fixed by the pattern, but `search2` computes each one only at
  its block step (`two_level::sym`, itself two dependent loads), so a lookahead over blocks would first need them
  computed ahead.
- **Switch:** `KATKA_LA_D` gives d; 0, the default, turns it off.

| d (BML, s; 16 / 32 / 64) | plain | ▷ |
|---|---|---|
| 1 | 4.31 / 4.47 / 4.85 | 3.32 / 3.40 / 3.67 |
| 2 | 4.31 / 4.47 / 4.83 | 3.33 / 3.41 / 3.69 |
| 4 | 4.33 / 4.48 / 4.85 | 3.34 / 3.42 / 3.70 |
| 8 | 4.37 / 4.52 / 4.89 | 3.37 / 3.44 / 3.72 |
| 16 | 4.43 / 4.59 / 4.97 | 3.44 / 3.51 / 3.79 |

d = 1 and 2 are from `time/d1`, 4–16 from `time/d`. Within each run the same-day current agreed with the summary's
within 1%.

## E. Chopping and interleaving: not done

Chopping and interleaving were to be tried on a row only if part (c) of X was at least 10% of cycles after C and D. It is 0.8 / 1.5% on the plain
row and 0.9 / 2.0% on the ▷ row (16 / 64), so E was skipped on both rows. No chopping and no interleaving were
implemented.

## Huge pages (`hp/`, `time/hp1`–`hp3`; `hp_extra.sh`, `hp_tools.sh`)

- **The machine:**
  - THP: `enabled` = `madvise`, `defrag` = `madvise`;
  - glibc 2.43;
  - katka reads its indexes with `fread` into `std::vector`s, not mapped from files.
- **Without the tunable, no memory is in huge pages:** `AnonHugePages` is 0 of 1.37 GB resident for plain two-level at
  16.
- **The route that worked:** `GLIBC_TUNABLES=glibc.malloc.hugetlb=1`, which makes glibc's malloc ask for transparent
  huge pages on its large allocations. No code change and no root were needed, so `madvise` in the loader was not
  tried.

**Times with and without** (BML, s, 16 / 32 / 64; same-day runs interleaved; `time/hp.table.md`, FB there too):

| row | without | with huge pages | gain |
|---|---|---|---|
| plain move structure | 19.26 / 21.33 / 22.95 | 15.97 / 17.69 / 18.95 | 17 / 17 / 17% |
| two-level plain, current | 6.47 / 6.58 / 7.09 | 5.58 / 5.88 / 6.55 | 14 / 11 / 8% |
| two-level plain, table | 5.72 / 5.76 / 6.45 | 4.92 / 5.20 / 5.77 | 14 / 10 / 11% |
| two-level plain, table + lookahead (d = 2) | 4.40 / 4.46 / 4.83 | 3.92 / 4.07 / 4.42 | 11 / 9 / 8% |
| ▷, current | 3.67 / 3.66 / 3.92 | 3.32 / 3.31 / 3.52 | 10 / 10 / 10% |
| ▷, table | 3.50 / 3.50 / 3.75 | 3.13 / 3.13 / 3.36 | 11 / 11 / 10% |
| ▷, table + lookahead (d = 2) | 3.33 / 3.40 / 3.68 | 2.96 / 3.03 / 3.27 | 11 / 11 / 11% |
| heavy paths θ = 0.9, current | 3.22 / 3.15 / 3.22 | 2.90 / 2.89 / 3.02 | 10 / 8 / 6% |
| heavy paths θ = 0.9, table | 3.11 / 3.05 / 3.11 | 2.75 / 2.74 / 2.98 | 12 / 10 / 4% |

- **The bimodality:** the plain row's "current" at 16 without huge pages is the median of runs of 6.47, 6.53 and 6.09 s
  (see the reproduced rows). With huge pages, its runs agree within 0.01 s.
- **Share of the process's memory in huge pages** (the largest `AnonHugePages` / `Rss` sampled during a run, 3 runs
  each; `hp/share.txt`):

  | row | 16 | 32 | 64 |
  |---|---|---|---|
  | plain move structure | 90.5–90.7% | 89.3–90.8% | 91.5–91.7% |
  | two-level plain, current | 81.6–81.8% | 64.6–75.9% | 49.7–56.4% |
  | two-level plain, table | 82.4–82.7% | 79.6–80.4% | 56.7–61.9% |
  | two-level plain, table + lookahead | 82.6–82.7% | 82.2–83.0% | 63.2–64.6% |
  | ▷, current | 81.7–81.8% | 82.5–83.4% | 66.2–68.1% |
  | ▷, table | 79.1–82.5% | 81.5–83.1% | 64.2–64.3% |
  | ▷, table + lookahead | 81.0–82.6% | 81.1–82.3% | 64.3–71.1% |
  | heavy paths, current | 81.0–83.7% | 70.4–71.7% | 75.4–81.4% |
  | heavy paths, table | 82.4–84.3% | 65.9–68.6% | 77.9–78.0% |

  The share varies between runs in some cells, by up to 11 points (plain, current, 32), and is lower at 64
  individuals. That is presumably fragmentation: huge pages depend on unfragmented free memory.
- **Load time** (`index loaded in`, s, 16 / 32 / 64), without → with huge pages:
  - plain move structure: 0.4 / 0.4 / 0.4 → 0.2 / 0.2 / 0.2;
  - two-level plain, current: 0.7 / 0.8 / 0.9 → 0.4 / 0.4 / 0.5;
  - with the table: 1.0 / 1.1 / 1.4 → 0.6 / 0.7 / 0.9;
  - ▷: the same as plain;
  - heavy paths: 0.8 / 0.9 / 1.0 → 0.4 / 0.4 / 0.6, and with the table 1.0 / 1.2 / 1.5 → 0.6 / 0.8 / 1.1.

  Load times fall: fewer page faults.
- **Page walks at 16,** share of the search's cycles, without → with (`hp/walks.txt`, `perf stat`, the 100-character
  run subtracted):

  | row | current | table | table + lookahead |
  |---|---|---|---|
  | two-level plain | 12.7 → 3.4% | 18.0 → 3.7% | 18.4 → 3.7% |
  | ▷ | 12.9 → 3.7% | 14.7 → 3.3% | 15.3 → 4.0% |
  | heavy paths | 14.1 → 7.3% | 15.4 → 7.2% | |
  | plain move structure | 17.2 → 0.1% | | |

- **Where the gain is largest:** on the plain move structure (17%), which also had the most page walks and is almost
  entirely in huge pages. On the two-level rows it is 8–14%, larger where page walks had risen with the table. It is
  smaller at 64 individuals, where less of the memory is in huge pages.

**ropebwt3 and Movi 2 with the same variable, 16 individuals** (`hp/tools.log`; see below).

| tool (16 individuals, BML, L = 1000, one thread) | without (3 runs; median) | with (3 runs; median) | gain | memory in huge pages |
|---|---|---|---|---|
| ropebwt3 3.10-r285, `mem -t 1 -l 1000` (query time as in `run_ropebwt3.sh`) | 70.39, 66.11, 65.78; **66.11** | 61.20, 60.36, 60.08; **60.36** | 9% | 86.7% of 346 MB |
| Movi 2, `query --mem --min-mem-length 1000 --ftab-k 10 -t 1` (its own "processing the reads") | 28.01, 26.46, 26.28; **26.46** | 24.47, 23.79, 23.29; **23.79** | 10% | 96.5% of 879 MB |

- **The runs:** with their October indexes and queries (copied from the external drive's
  `twolevel_runs/twolevel/results/n16/`), with and without interleaved in the same rounds. The outputs were identical
  with and without in every round.
- **The ropebwt3 and Movi 2 rows run slower than the paper's numbers** (64.5 and 25.9 s in October). Only the
  comparison with and without matters here.
- **Load times:** Movi 2's fell from 0.28 to 0.15 s.
- **Movi 2's share of memory in huge pages** is from one extra run: the timed runs' probe read the wrapper `movi`, not
  its worker `bin/movi-regular` (fixed in the script afterwards).
- **So the variable works for all three tools,** with no code change to any.

## The final timing session (`final/`; `final.sh`, `final_table.py`)

Every row of v3's Tables 1–9 that has a time, and the new rows, at 16 / 32 / 64 individuals, each without and with
huge pages (`GLIBC_TUNABLES=glibc.malloc.hugetlb=1`), on 2026-11-22.
- **Conditions:** the machine was rebooted at 08:08 and the session ran from 08:53 to 12:38 with nothing else running
  (load average 0.04 at the start).
- **Rules:**
  - one thread pinned to CPU 0, an 11 GB cgroup without swap, load time excluded, the median of 3 rounds;
  - each round runs every row in both modes, and which mode goes first alternates from row to row and from round to
    round;
  - a row whose 3 times without huge pages differed by more than 3% got two more runs (median of 5, marked † in
    `final/tables.md`).
- **The times:**
  - katka: `find`'s `(total … s)`, as `experiments/twolevel_ac_table.py`;
  - ropebwt3: from "loaded the BWT" to "processed";
  - Movi 2: its "processing the reads";
  - the STPD-index driver: its "finding them".
- **The rows:**
  - katka's rows with the paper's options and the unchanged binary (`bin/katka-base`):
    - Table 1: RLCSA and the move structure;
    - Table 2: two-level on the RLCSA and on the move structure;
    - Table 4: our STPD of the parse;
    - Tables 5 and 6: almost-funnels on the text and from CHM13's parse;
    - Table 7: heavy paths, all three θ;
    - Table 8: ▷;
  - ropebwt3 3.10-r285 (`mem -t 1 -l 1000`; FB `--old-mem`) and Movi 2 at 9bc0acc (`query --mem --ftab-k 10 -t 1`,
    `--min-mem-length 1000`; the ∗ column with 1, its own query for all MEMs), with their October indexes, re-run
    under these rules (the paper's rows for them were from earlier runs, a median of 5 with no memory limit);
  - Cenzato et al.'s STPD-index (RLZ + r-index, `bin/stpd-index-mems`) at 16 and 32 individuals;
  - the new rows:
    - two-level with the first-run table (`bin/katka-table`), with the table and lookahead d = 2, and with lookahead
      alone d = 4;
    - ▷ with the table, and with the table and lookahead d = 2;
    - heavy paths, all three θ, with the table.
- **Checks, all passed:**
  - every output is byte-identical to the first run of the same row, size and algorithm, in both modes;
  - for the new rows, it is byte-identical to the unchanged binary's output;
  - katka's and the STPD-index's MEMs (and katka's intervals) equal the paper's `blocks/q<N>s.blocks.tsv`: 22,711 /
    18,479 / 16,806 MEMs;
  - ropebwt3's and Movi 2's outputs at 16 are byte-identical to the huge-page trial's.
- **Files:**
  - `final/tables.md`: the tables, twice each;
  - `final/runs.tsv`: every run's time, share in huge pages, resident size and peak;
  - `final/logs/`: the logs (`q<N>.<row>.<alg>.<off|hp>.<round>.{log,time,hp}`);
  - `final/final.log`: the driver's log.

**Summary of BML times at 16 / 32 / 64 individuals (s), from `final/tables.md`:**

| row | posted v3 | today, without | today, with huge pages |
|---|---|---|---|
| RLCSA | 43.3 / 53.8 / 55.4 | 39.87† / 48.48 / 53.09 | 33.30 / 39.26 / 42.63 |
| our move structure | 20.26 / 22.65 / 23.68 | 19.25 / 21.37 / 22.98 | 15.99 / 17.70 / 18.98 |
| ropebwt3 | 64.3 / 72.8 / 79.5 | 65.44 / 73.89 / 80.70 | 59.94 / 67.72 / 73.80 |
| Movi 2 | 25.9 / 30.2 / 35.5 | 26.25† / 30.58 / 35.90 | 23.26 / 27.13 / 32.07 |
| STPD-index | 43.56 / 48.73 / — | 39.07 / 44.08 / — | 38.64 / 43.58 / — |
| two-level, move structure | 6.56 / 6.96 / 7.75 | 6.03† / 6.43 / 7.15† | 5.53 / 5.87 / 6.48 |
| with table and lookahead d = 2 (new) | | 4.29 / 4.45 / 4.83 | 3.91 / 4.08 / 4.39 |
| heavy paths, θ = 90% | 3.41 / 3.34 / 3.34 | 3.20 / 3.15 / 3.21 | 2.86 / 2.80 / 2.86 |
| heavy paths, θ = 80%, with the table (new) | | 3.10 / 3.01 / 3.08 | 2.75 / 2.65 / 2.69 |
| ▷ | 3.91 / 3.94 / 4.22 | 3.65 / 3.66 / 3.92 | 3.31 / 3.30 / 3.50 |

- **Against the posted paper,** without huge pages:
  - katka's rows are 3–10% faster;
  - ropebwt3 and Movi 2 are 1–2% slower;
  - the STPD-index is 10% faster with BML and 12% with FB (59.43 against 67.62 s at 16).
  - The paper's runs were taken on several earlier days, so these differences are between days, not between versions.
- **Huge pages:**
  - 15–20% faster on the RLCSA and the move structure, with or without almost-funnels on the text;
  - 8–13% on the two-level rows, including the STPD of the parse;
  - 8–9% on ropebwt3 and 11% on Movi 2;
  - about 1% on the STPD-index.
- **Every new row is faster than its base row in both modes.** The best row is heavy paths with the table: θ = 80% at
  32 and 64 individuals. At 16, θ = 80% and θ = 90% tie with the table (3.10 against 3.12 s without huge pages, both
  2.75 s with them).

**Stability.**
- **Spread:** over the first 3 runs of each of the 154 row × size × algorithm cells, (max − min) / min has a median of
  0.47% without huge pages and 0.53% with them.
- **Over 3%:** 10 cells without and 7 with.
  - The largest are the session's first runs (RLCSA at 16: 43.20 then 39.74, 39.87; Movi 2's first run at 16: 27.48
    then 26.2–26.4).
  - Otherwise, one run of the cell is 3–7% slower than the others.
- **So after the reboot both modes were stable.** The bimodality without huge pages seen on 2026-11-21/22 (about 6.1
  or 6.5 s on the plain two-level row at 16, before the reboot) did not recur: today's 5 runs of that row were
  6.01–6.23 s.
- The cells over 3% are listed with all their runs in `final/tables.md`, "Stability".

**Shares of resident memory in huge pages** (with huge pages; `final/tables.md` has every row):
- 82–87% for the two-level rows, 86–90% for heavy paths and 86–92% for the move structure;
- 92–97% for the RLCSA, 87–97% for ropebwt3, 98% for Movi 2's worker and 96–97% for the STPD-index;
- the STPD of the parse varies the most, 81–95%.
- The shares are higher than in the trial at 64 individuals (50–81% there). Without huge pages they are 0.

**Headline ratios,** plain move structure over the best two-level row, 16 / 32 / 64:

| mode | BML, the paper's rows | BML, with the new rows | FB, the paper's rows | FB, with the new rows |
|---|---|---|---|---|
| without huge pages | 6.01 / 6.87 / 7.27 | 6.20 / 7.10 / 7.47 | 5.94 / 6.96 / 7.47 | 6.07 / 7.14 / 7.66 |
| with huge pages | 5.58 / 6.40 / 6.77 | 5.82 / 6.67 / 7.06 | 5.56 / 6.51 / 6.94 | 5.71 / 6.67 / 7.15 |

- The best two-level row is heavy paths: θ = 90% (paper's rows, 16, BML), otherwise θ = 80%. With the new rows it is
  θ = 80% with the first-run table, except at 16, where it ties with θ = 90% (see above).
- **Huge pages lower the ratio** by 5–7%, because they help the move structure more (17%) than heavy paths (11–13%).

**Sizes.**
- **The paper's MB column** is the size of the files each search loads (10^6 bytes), plus the 2 MB table of 8-mer
  cursors that `--move` builds. This is `experiments/twolevel_ac_table.py`'s count, and today's MB match v3's to within
  1 MB.
- **The new rows load the same files as their base rows,** so their MB entry is the base row's.
- **The first-run table** is built at load and is not in any file. It takes 77.9 / 90.4 / 108.7 MB (78.8 / 91.0 /
  108.9 for ▷), from the log's "first-run tables" line.
  - Of that, the parse's part is 68.1 / 76.2 / 88.1 MB; the rest is the blocks' table, which is built whenever the
    index has blocks.
  - The no-jumps and heavy-path searches never use the blocks' table.
- **Lookahead alone** builds no table.

**Rows that could not be reproduced:** none. The STPD-index has no 64-individual row, as in the paper, because its
construction does not fit in 16 GB.

## Files

- `repro/`: the reproduced rows.
- `a/`: A's histograms (`a_hist.txt`), MEM lengths (`a3_mems.txt`) and the counting runs' logs.
- `b/`: B's counters (`*.csv`), class shares (`*.class.txt`), per-line shares (`*.lines.tsv`) and summaries
  (`b_summary.txt`; `b_summary.cd.txt` with C and D); the class shares with `B_STRICT=1` (`*.class.strict.txt`, which
  correct the text index's share in two-level rows); the classifier's source copies and the perf data are in
  `~/data/latency/b/`.
- `time/`: every timed run's log (`<set>/q<N>.<row>.<variant>.<alg>.<round>.log`) and each set's table
  (`<set>.table.md`):
  - `c`: current and table, three rows;
  - `d`: d = 0, 4, 8, 16;
  - `d2`: d = 2 and 4;
  - `d1`: d = 1 and 2;
  - `alone`: the lookahead without the table;
  - `hp1`–`hp3`: with and without huge pages.
- `hp/`: shares in huge pages, page walks, and the ropebwt3 and Movi 2 runs.
- `final/`: the final timing session (tables, every run, logs).
