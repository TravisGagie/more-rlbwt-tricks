# Locating from the parse alone with a regular sample (2026-11-26)

**The design:**
1. The forward search that finds a MEM keeps the parse interval where it leaves the parse (`../forward_check.txt`:
   the same as the MEM's own search for every MEM).
2. It keeps the rows whose preceding phrase agrees with the piece of the MEM left of the interval, run by run. The
   comparison allows for the 51-character overlap.
3. Each remaining row walks forward with Ψ to a sampled row, adding the phrase's length minus 51 at each step.

It is built on the text-free index, `c<N>tl` (stage 2, part C: `c<N>tl.lvl` + `c<N>tl.tl2` in `~/diamond`). That
index's search already compares the left piece with each distinct phrase preceding the interval, run by run, to
count the occurrences. Locating keeps the rows that search found and walks them.

**Code** (branch `textless`):
- **`src/parse_sample.hpp`, the regular sample.** The rows of the parse's BWT whose phrase index is a multiple of
  s = 64 are marked, along with the last phrase's row and $'s. The marks are an Elias–Fano array, and each sampled
  row stores the text position where its phrase starts. `locate` walks Ψ over the two-level index's parse BWT.
- **`src/textless.hpp`:** `search_textless` records where it left the parse, but only when `g_tl_left.on` is set.
- **`src/mems.hpp`:**
  - `segment_search::ploc_` collects the rows run by run and walks them. The positions of rc(MEM) go to `occbuf`,
    and the two parts are timed separately.
  - BML locates after its forward search; forward–backward locates after a forward search whose MEM it reports.
- **`find --parse-locate FILE.prs`**, with `--textless`. `--locate-out` writes the positions. `$KATKA_PLOC_CHECK`,
  set to the parse (`c<N>.ptxt`), checks every position. Without the option nothing changes: the output is the same.
- **`bin/prs-build LVL PTXT OUT.prs [s]`** (`experiments/parse_locate/prs_build.cpp`) uses the parse's suffix array by
  libsais. It then walks from 2,000,000 random rows and checks each result against the suffix array.
- **`bin/katka-ploc`** is `src/main.cpp` at this commit; it is used for every timed run, with and without locating.

## Checks (`checks.txt`, `check.*.log`, `build.*.txt`; `experiments/parse_locate/regular_check.sh`)

**Building:** the walks from 2,000,000 random rows gave 0 wrong positions at 16, 32 and 64 individuals, with 31.5 Ψ
steps per row.

**Searching** (BML and forward–backward, L = 1000, every reported MEM):

| individuals | MEMs | positions | Ψ steps per position | checked against the text | against two-level `--locate-out` |
|---|---|---|---|---|---|
| 16 | 22,711 | 72,876 | 31.46 | all correct and distinct | equal |
| 32 | 18,479 | 64,945 | 31.48 | all correct and distinct | equal |
| 64 | 16,806 | 75,563 | 31.54 | all correct and distinct | (no `c129.tloc`) |

- **The MEMs and occurrences** equal plain search's in every run.
- **Checked against the text** means every position holds rc(MEM): the full position, the phrase start minus the left
  piece, compared over the MEM's whole length by the parse oracle. Each MEM's positions are distinct, and there are
  as many as its occurrences. So each MEM's set of positions is exactly its set of occurrences.
- **At 16 and 32,** the positions equal two-level search's `--locate-out` (`chr19/locpos/tlpos1000.$N.txt`), MEM by MEM.

**L = 100** wasn't run. The minimal-width index's own search stops there, without locating
(`stopped.bml.33.100.log`: "alpha has no identifier in an exact search"); this is part C1's conservative rule. The
search has no MEMs with only the right end at the parse level that need a fallback. Its right end gives the parse
interval directly, and the rows are kept and walked in the same way.

## Sizes (MB), from `check.bml.$N.1000.log`

| | 16 | 32 | 64 |
|---|---|---|---|
| regular sample, s = 64: rows marked (Elias–Fano) | 4.145 | 8.162 | 16.208 |
| regular sample: text positions | 13.779 | 27.984 | 57.253 |
| **regular sample, in all (the only addition)** | **17.924** | **36.146** | **73.460** |
| run heads of the parse BWT in row order (already in `c<N>tl.tl2`) | 30.996 | 38.863 | 52.348 |
| their rows (already in `c<N>tl.tl2`) | 9.956 | 13.465 | 19.626 |

The run heads and their rows are the text-free index's "parse BWT by row", which its search needs to count. They are
part of its 159.7 / 189.9 / 248.9 MB `.tl2` (`../../textless/c1/README.md`).

## Time (`time/`, `rerun33/time/`; `experiments/parse_locate/regular_time.sh`)

**How it was timed:** the paper's rules.
- One thread pinned to CPU 0, an 11 GB cgroup without swap, no huge pages.
- Loading excluded: find's "(total X s)".
- The median of 3 rounds, with the order of the runs with and without locating alternating.
- The same binary throughout. Every output was byte-identical to the first one of its configuration.

**At 16 individuals,** the first session's text-free runs spread by up to 17% (`time/33.*`). The numbers below are
from a rerun of 16 individuals with 5 rounds (`rerun33/`), whose spread is 0.2–1.8%. At 32 and 64 the spread is
under 2%.

**Added time, L = 1000, median (s):**

| individuals | text-free BML: without → with | **added** | text-free FB: without → with | **added** | two-level BML `--locate` added | two-level FB `--locate` added |
|---|---|---|---|---|---|---|
| 16 | 5.393 → 5.855 | **0.462** | 5.900 → 6.401 | **0.501** | 3.910 → 5.370: 1.460 | 3.921 → 5.375: 1.454 |
| 32 | 5.863 → 6.282 | **0.419** | 6.415 → 6.831 | **0.416** | 3.790 → 5.151: 1.361 | 3.748 → 5.112: 1.364 |
| 64 | 6.606 → 7.051 | **0.445** | 7.156 → 7.602 | **0.446** | (no `c129.tloc`) | |

Two-level `--locate` is `--csa --twolevel` (two-level search with blocks on the index of the text, `c<N>.tloc`). It
includes the second search per MEM.

**The split** (the runs with locating, every round):

| individuals | collecting the rows, run by run | Ψ walks | the rest of the added time |
|---|---|---|---|
| 16 | 0.003 s | 0.39–0.40 s | about 0.07 s |
| 32 | 0.002 s | 0.34 s | about 0.08 s |
| 64 | 0.002 s | 0.36 s | about 0.08 s |

- **Collecting the rows** reads 19,495 / 16,287 / 15,418 runs in all. The MEMs with thousands of almost-MEMs don't
  matter: the comparison with each preceding phrase is part of the search without locating, which needs it to count
  the occurrences, and reading the runs again takes 2–3 ms.
- **The Ψ walks** take 171 / 165 / 151 ns per step. That's less than the 194–230 ns
  measured from random rows (`../samples.$N.txt`), including the check for a sampled row at each step.
- **The rest** is recording the left end at every forward search, the copies of its phrases and agreements, whether
  its MEM is reported or not, plus the timer calls.
