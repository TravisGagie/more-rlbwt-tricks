# Locating from the parse alone: survivors, samples and Ψ (2026-11-25/26)

The question: when two-level search with blocks finishes its parse steps for a reported MEM, how many rows are left
(the **survivors**), compared with the MEM's occurrences (occ)? Also the parse's sizes, the structures a φ⁻¹ or a
regular sample over the parse would need, and the gaps between the parse run samples. Counting only; nothing new is
in the index and nothing is timed except one Ψ step.

**Code** (branch `textless`):
- **`bin/katka-surv`** is katka with `-DKATKA_SURV` (`src/surv_count.hpp`, with `#ifdef` hooks in `src/twolevel.hpp`'s
  `search2` and `src/mems.hpp`'s `rc_interval`). `bin/katka` is not rebuilt and is unchanged.
  - For every reported MEM, it runs the MEM's own two-level search again, as `find --locate` does. It writes the
    parse interval [plo, phi] when that search leaves the parse, before the character steps at the left end, one line
    per MEM.
- **`experiments/parse_locate/`:**
  - `build.sh` builds `bin/katka-surv` and `bin/parse-samples`;
  - `run.sh` runs the searches;
  - `summarize.py` writes `summary.md`;
  - `parse_samples.cpp` measures the samples and Ψ.
- **How it ran:** one run at a time, pinned to CPU 0, in an 11 GB cgroup.
- **Settings:** BML, `--csa --twolevel` (two-level search with blocks, the configuration of `--locate`),
  `chr19/blocks/queries.fa`, L = 1000 and 100.
- **The first attempt at 64 individuals** was stopped by the cgroup at 11.5 GB, because the tool made a temporary
  copy of the phrase starts. That copy is gone in the final version, whose peak was 8.2 GB.

## Checks (`checks.txt`)

Every run's MEMs (columns 1–5) equal plain search's (`chr19/plain$L.$N.tsv`). Each MEM's occ is equal to ep − sp of
the run, and survivors ≥ occ holds for every MEM.

| individuals, L | MEMs | sum of occ |
|---|---|---|
| 16, 1000 | 22,711 | 72,876 (the committed number) |
| 16, 100 | 38,984 | 384,341 (the committed number) |
| 32, 1000 | 18,479 | 64,945 |
| 32, 100 | 30,809 | 362,636 |
| 64, 1000 | 16,806 | 75,563 |
| 64, 100 | 27,137 | 393,185 |

## Survivors (`summary.md`, from `blocks.$N.$L.tsv.gz`)

| individuals, L | MEMs with parse steps | without (right end only) | sum of occ | sum of survivors | ratio | survivors = occ |
|---|---|---|---|---|---|---|
| 16, 1000 | 22,711 | 0 | 72,876 | 94,569 | 1.30 | 22,140 |
| 32, 1000 | 18,479 | 0 | 64,945 | 102,179 | 1.57 | 17,933 |
| 64, 1000 | 16,806 | 0 | 75,563 | 162,410 | 2.15 | 16,272 |
| 16, 100 | 38,927 | 57 | 383,007 | 8,958,153 | 23.4 | 35,004 |
| 32, 100 | 30,770 | 39 | 360,561 | 13,794,404 | 38.3 | 27,181 |
| 64, 100 | 27,103 | 34 | 391,281 | 17,752,684 | 45.4 | 23,917 |

- **The sums of occ in this table** are over the MEMs with parse steps only.
- **The MEMs without parse steps** are 100–117 characters long. Each has a phrase boundary, but the only piece at
  the parse level is the right end. Their survivors are then every row that starts at a phrase boundary with that
  piece: 209,100 for 1,334 occurrences at 16 individuals.
- **No MEM lacks a phrase boundary altogether.**
- **The distributions** (mean, median, 90%, 99% and maximum of occ, survivors, survivors / occ and survivors − occ)
  are in `summary.md`.
  - At L = 1000 the median ratio is 1 and the 99th percentile is 2.9–3.2.
  - The mean is pulled up by a few MEMs with thousands of almost-MEMs (at most 11,240 / 33,631 / 81,644).

**Parse rows, not block rows.** The survivors are counted as the contiguous parse interval [plo, phi], since
parse-only locating walks parse rows.
- **Not all of them start blocks:** the block rows among them sum to 33,147 / 54,226 / 111,236 at L = 1000.
- **Fewer than occ:** those sums are smaller than occ. Counting only block rows would miss occurrences.

**▷ and heavy paths θ = 80% give the same rows** (`unique.33.*`, `heavy08.33.*`). At 16 individuals, L = 1000 and
100, every MEM's kind, survivors and left piece are equal to those of two-level search with blocks.

**The left piece always lies inside one phrase.**
- **What the left piece is:** the part of the MEM left of its first full phrase.
- **In every MEM at every size and L,** no phrase start of the pattern falls inside it, so it lies within the phrase
  that precedes the row in every occurrence.
- **The parse level never stopped** at a phrase that is not a phrase of the text.
- **Its length** is 11.5–12.6 characters on average and 38 at most.
- **So** checking one survivor takes one look at the phrase preceding its row, BWT_Z at that row.
- **The overlap:** phrases overlap by exactly K = 51 characters (phrase length minus the distance to the next start
  is 51 for every phrase but the last). So the check compares the left piece with the characters of the preceding
  phrase that end 51 characters before its end.

## The parse and the structures over it (`samples.$N.txt`)

All in MB, by the current code's accounting.

| | 16 | 32 | 64 |
|---|---|---|---|
| n | 4,065,485,531 | 8,004,217,317 | 15,897,786,191 |
| phrases m | 220,466,619 | 434,172,121 | 862,158,111 |
| distinct phrases | 8,512,838 | 9,522,052 | 11,011,641 |
| r_Z (runs of the parse BWT) | 10,332,018 | 12,954,298 | 17,449,211 |
| **phrases per run, m / r_Z** | **21.3** | **33.5** | **49.4** |
| blocks; r_B | 23,158,046; 2,613,788 | 52,209,743; 4,548,667 | 120,622,565; 8,009,328 |
| parse run samples, phrase indices (rows + SA_Z) | 46.1 | 60.4 | 85.1 |
| the same with text positions | 51.3 | 66.9 | 93.8 |
| φ⁻¹ over the parse, phrase indices | 46.1 | 60.4 | 85.1 |
| φ⁻¹ over the parse, text positions | 56.8 | 73.7 | 103.2 |
| phrase → text | 205.6 | 404.8 | 803.9 |
| block run samples, block indices | 10.3 | 18.7 | 34.5 |
| the same with text positions | 12.5 | 22.7 | 41.5 |
| block → phrase | 18.8 | 41.1 | 95.5 |
| regular samples every 64th phrase (rows marked + text positions) | 17.9 | 36.1 | 73.5 |
| the same every 16th / 32nd / 128th | 68.3 / 35.0 / 9.2 | 137.8 / 70.6 / 18.5 | 280.4 / 143.6 / 37.6 |
| run heads of the parse BWT in row order (to read BWT_Z at a row; + the rows, 10.0 / 13.5 / 19.6) | 31.0 | 38.9 | 52.3 |

`PREFIX.tloc` as built (the build logs, `results/chr19_stpd_parse/README.md`), in MB:

| | 16 | 32 |
|---|---|---|
| character run samples | 273.8 | 299.4 |
| φ⁻¹ of the text | 350.2 | 387.8 |
| parse run samples | 45.5 | 59.6 |
| phrase → text | 191.8 | 377.7 |
| block run samples | 10.1 | 18.4 |
| block → phrase | 17.4 | 37.9 |
| in all | 888.7 | 1,180.8 |

- **Loaded by the current code,** the same files count a little more: 354.4 for φ⁻¹, 46.1 for the parse run samples
  and 205.6 for phrase → text at 16 individuals. The other tables use the current code's counts throughout.
- **64 individuals:** `c129.tloc` was never built.

**Checks of the two claims about text positions:**
- **φ⁻¹ over the parse in text positions is exact.** It gives SA at the next row, in phrase indices and in text
  positions, at every row of the parse's suffix array but the last: 0 wrong at all three sizes.
- **Ψ over the parse BWT is exact.** Ψ(i) is the start of the run holding F-row i, plus i − FP. It and the phrase it
  passes give the next suffix's text start: the start plus the phrase's length minus 51. This holds at every row but
  the one of the last phrase, which ends the text and so has no overlap.

## The forward search already leaves the MEM's parse interval (`forward_check.txt`, 2026-11-26)

**The question:** a search with no toehold still needs the MEM's parse interval at the point where it leaves the
parse. Does the forward search that found the MEM leave it there, so that no second search is needed?
- **The argument:** BML and forward–backward both find a MEM's end by a forward search from its start (BML after its
  jump and left extension). That search has the same right end and the same phrases as the MEM's own search. A parse
  step over the phrase crossing the MEM's end would mean the match is longer, so the search leaves the parse at the
  same phrase, with the same interval.
- **The check** (`experiments/parse_locate/check_forward.sh`; `bin/katka-surv` records where `forward` leaves the parse
  and `rc_interval` compares it, by kind, plo and size): BML and `--fb`, 16 / 32 / 64 individuals, L = 1000 and 100.
  Every reported MEM, 22,711 / 18,479 / 16,806 at L = 1000 and 38,984 / 30,809 / 27,137 at L = 100, has the same
  parse interval in both, including the MEMs with only the right end at the parse level. Every reported MEM came from
  a forward search from its start, and the MEMs equal plain search's in every run.

## Ψ timing (`samples.$N.txt`)

**Method:** 200,000 random rows (fixed seed), each followed by 64 dependent Ψ steps, each adding the phrase's length
minus K, repeated 3 times.
- The structure is the two-level index's parse BWT: runs grouped by phrase, Elias–Fano over their positions in F.
  There is no RLCSA of the parse.
- Pinned to CPU 0.

| individuals | ns per step |
|---|---|
| 16 | 194–195 |
| 32 | 202–203 |
| 64 | 230 |

## Gaps between the parse run samples, and subsampling (`samples.$N.txt`)

The samples are SA_Z at the starts of the parse BWT's runs.

**Gaps in phrase indices.** Counts of gaps of each size between consecutive samples:

| | 1 | 2 | 3 | 4 | 5–8 | 9–16 | 17–32 | 33–64 | > 64 |
|---|---|---|---|---|---|---|---|---|---|
| 16 | 8,086,909 | 281,187 | 183,347 | 129,155 | 303,996 | 285,396 | 283,694 | 251,024 | 527,309 |
| 32 | 8,990,473 | 435,501 | 290,080 | 210,065 | 506,811 | 492,380 | 501,844 | 460,938 | 1,066,205 |
| 64 | 10,391,030 | 718,977 | 479,902 | 353,521 | 858,847 | 847,417 | 881,630 | 838,060 | 2,079,826 |

**Subsampling** (Cobas, Gagie and Navarro, Section 4.1), the fraction of samples that remain:

| s | 2 | 4 | 8 | 16 | 32 | 64 |
|---|---|---|---|---|---|---|
| 16 | 0.631 | 0.430 | 0.310 | 0.230 | 0.171 | 0.125 |
| 32 | 0.680 | 0.500 | 0.387 | 0.304 | 0.238 | 0.181 |
| 64 | 0.733 | 0.577 | 0.470 | 0.385 | 0.312 | 0.245 |

**In characters,** the bounds and s are those in phrases times 18, from n / m = 18.44. The fractions that remain for
s = 36 … 1152 are 0.723 … 0.127 at 16 individuals, 0.757 … 0.184 at 32 and 0.795 … 0.248 at 64. The histograms are
in `samples.$N.txt`.
