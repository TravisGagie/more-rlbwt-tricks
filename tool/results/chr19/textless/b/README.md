# Two-level search without the index of the text, BML, L = 1000 (stage 2, part B; 2026-11-23)

*Commit hashes in this README refer to the private development history of this code; those commits are not in this repository.*

Code: branch `textless`.
- `src/textless.hpp` (`search_textless`, `textless_index`), `find --textless` (`src/main.cpp`, `src/mems.hpp`).
- `PREFIX.tlx` is built by `experiments/textless/build.cpp`.
- Drivers: `experiments/textless/check.sh` (verification) and `experiments/latency/final.sh` (rows `textless`,
  `textless+table+la2`; timing). Tables: `experiments/textless/time_table.py`.
- The index: the paper's plain two-level `c<N>.lvl` (only its parse's BWT and dictionary are used) and `c<N>.tlx`.

## Verification before timing (`check/`)

`KATKA_TEXTLESS_CHECK=1` loads the index of the text as well, and repeats every search with two-level search
(`search2`). At 16, 32 and 64 individuals, both strands, BML:
- **j:** 0 differences, in 93,190 / 72,494 / 64,369 searches.
- **The number of occurrences:** 0 differences against the interval's size.
  - In 6,074 / 3,803 / 2,915 window tests, the tail does not occur and the search stops there, with a shorter skip
    (below).
  - In 52 / 27 / 18 of them, that shorter j happens to be the exact one, but no count is made.
- **The output:** the MEMs (columns 1–3) are byte-identical to the final timing session's plain two-level output, and
  each MEM's number of occurrences equals its interval's size there.
- **No search lacked a phrase boundary,** and no left-end comparison agreed over a whole phrase (`bug_full_agree` = 0).

## Cases (`check/q<N>.check.log`, "textless" lines)

| | 16 | 32 | 64 |
|---|---:|---:|---:|
| searches (window tests / exact) | 93,190 (47,768 / 45,422) | 72,494 (35,536 / 36,958) | 64,369 (30,757 / 33,612) |
| the K-mer's range from alpha | 92,182 | 72,029 | 64,065 |
| from alpha cut by the window's start | 0 | 0 | 0 |
| by binary search over the ranges (alpha has no identifier) | 1,008 | 465 | 304 |
| — of which the K-mer is not a phrase's first K-mer | 870 | 397 | 258 |
| the tail is just the K-mer (no comparison) | 11,587 | 10,020 | 9,230 |
| string comparisons at the right end | 527,146 | 466,576 | 454,490 |
| the tail does not occur (all in window tests) | 6,126 | 3,830 | 2,933 |
| window tests with a shorter skip: searches, mean characters lost | 6,074, 38.5 | 3,803, 39.0 | 2,915, 39.7 |
| the tail occurs, not after alpha (alpha's step fails) | 308 | 260 | 227 |
| left ends | 87,064 | 68,664 | 61,436 |
| — at the window's start | 22,716 | 18,484 | 16,811 |
| — one distinct preceding phrase | 88.1% | 86.3% | 85.3% |
| — two | 5.4% | 6.7% | 7.3% |
| — more than 64 | 78 | 75 | 77 |
| — the most | 1,239 | 1,563 | 2,177 |

## Times (`time/times.md`; BML, one session; 16 / 32 / 64)

| | without huge pages | with huge pages |
|---|---|---|
| two-level, move structure (the paper's) | 6.05 / 6.47 / 7.17 | 5.62 / 5.89 / 6.55 |
| **two-level without the index of the text** | **5.49 / 5.95 / 6.73** | **5.06 / 5.51 / 6.26** |
| two-level, table + lookahead d = 2 | 4.31 / 4.46 / 4.84 | 4.00 / 4.11 / 4.44 |
| **without the index of the text, table + lookahead d = 2** | **3.73 / 3.97 / 4.38** | **3.47 / 3.70 / 4.08** |
| ▷ (the paper's) | 3.67 / 3.67 / 3.95 | 3.31 / 3.31 / 3.51 |
| ▷, table + lookahead d = 2 | 3.33 / 3.41 / 3.72 | 3.01 / 3.06 / 3.26 |

- **Speed:** without the index of the text, search is 6–10% faster plain and 9–13% faster with the table and lookahead.
- **Memory:** the largest resident size is 836 / 914 / 1,041 MB against 1,414 / 1,525 / 1,698 MB (without huge pages).
- **The re-timed rows** match the committed medians to within 2.3%.
- **Files loaded:** the parse's BWT and the dictionary of `c<N>.lvl`, 154.9 / 181.2 / 223.6 MB, plus `c<N>.tlx`, 270.8 /
  309.8 / 373.2 MB. That is 425.7 / 491.0 / 596.8 MB, against the paper's 1,068 / 1,183 / 1,362.
  - The `.tlx` stores whole phrases and fixed-width offsets and ranges. The encoding of part A would take 368 / 424 /
    516 MB in all.
  - `c<N>.lvl`'s B (13–18 MB) and its unused pairs are read but not used.

## Profile (`profile/`; `perf record -e cycles:u`, `bin/katka-tl-g`, `B_STRICT=1`; shares of cycles after the load)

| | plain 16 / 32 / 64 | table + lookahead 16 / 32 / 64 |
|---|---|---|
| the parse's BWT (parse steps) | 70.5 / 73.4 / 75.4% | 42.1 / 44.5 / 46.8% |
| the lookahead's prefetches | | 7.8 / 8.7 / 9.8% |
| parse steps, other (the loop) | 1.1 / 1.1 / 1.1% | 6.3 / 6.7 / 6.7% |
| pattern parsing | 20.1 / 18.0 / 16.7% | 31.5 / 28.6 / 26.2% |
| **the right end (lookup)** | **1.4 / 1.1 / 0.9%** | **1.9 / 1.4 / 1.2%** |
| **the left end (comparisons)** | **1.2 / 1.0 / 0.9%** | **1.6 / 1.6 / 1.3%** |
