# Almost-funnels on the text, both strands, on the move structure (2026-11-12)

*Commit hashes in this README refer to the private development history of this code; those commits are not in this repository.*

## What was built
- **Index:** our move structure `c<N>.mv` (5c576a8's `--move` text index) of the BWT of S$, which holds every sequence and its reverse complement. It's the one index for both search directions, as for all our other indexes. The move structure was feasible, so no RLFM-index was needed.
- **Almost-funnels:** Reportable Shortcuts' construction (`rs_build/reportable-shortcuts/src/build.cpp`), reimplemented in `src/tshortcut.hpp` with θ = 0 (no pruning):
  - **Along both strands:** almost-funnels go along CHM13 (the first sequence of S) and along rc(CHM13).
  - **Anchors:** rc(CHM13)'s anchors are mirrored, as Reportable Shortcuts aligns its reversed index's anchors. Anchor p of rc(CHM13) sits at CHM13's boundary |R| − p.
  - **Collisions:** windows with equal intervals are discarded across both strands, so entrances stay disjoint.
  - **Storage:** CHM13 is stored once, at 2 bits per character, and rc(CHM13) is read from it by complement.
  - **Run bits:** one extra bit per move-structure run marks runs holding a row of some entrance, so most steps skip the entrance lookup.
- **Configurations:**
  - fixed-length, windows of ℓ = 25 and ℓ = 100, τ = 0;
  - variable-length, windows of ℓ = 25, τ = 6.
- **Commands:** `katka index --text-shortcuts ELL,TAU` (`experiments/tsc_text_build.sh`), and `katka find --move --text-shortcuts FILE` for plain BML/FB.
  - A search at X with |X| ≥ ℓ whose interval lies in an entrance compares the pattern with α_{p,top}. It then takes the largest tier that matches and whose interval is nonempty (Lemma 3), as in Reportable Shortcuts' `search_range`.

| build (windows, τ) | anchors | discarded (colliding) | entrances | entrance rows at 16 / 32 / 64 |
|---|---:|---:|---:|---:|
| 25, 0 and 25, 6 | 4,936,586 | 907,574 | 4,029,012 | 250.5 M / 491.8 M / 975.5 M |
| 100, 0 | 1,234,144 | 55,008 | 1,179,136 | 67.2 M / 134.5 M / 267.8 M |

- **Build cost:**
  - **Time:** 35–116 s per build, single-threaded.
  - **Peak memory:** at 64, 10.1 GB (25, 0) and 10.5 GB (25, 6), close to the 11 GB cap. The cause is the wavelet matrix's 32-bit working copies of the ~975 M row symbols, and it could be cut if needed.
  - **Logs:** `build_logs/`, `build_run.log`.

## Timings
- **Setup:** `experiments/twolevel_ac.sh "33 65 129" "rlcsa.move tsc25t0 tsc100t0 tsc25t6" 3` with O = this directory.
  - The binary is `bin/katka` at 6313a58.
  - The inputs are the two query haplotypes, L = 1000, `-j 1`, pinned to CPU 0, in 11 GB cgroups without swap, median of 3, excluding load.
  - Each round runs every configuration once. All ran on 2026-11-12, 21:45–22:02.
- **Baseline:** the first row is plain search on the same index, the move structure alone, run the same day.
- **Checks:** every run's MEMs *and intervals* (columns 1–5) equal the paper's BML output (`blocks/q<N>s.blocks.tsv`: 22,711 / 18,479 / 16,806 MEMs).
- **MB loaded:** `c<N>.mv`, plus the 8-mer cursor table built at load (2.1 MB), plus the `.tsc` file.

| configuration | 16: BML / FB (s) | MB | 32: BML / FB (s) | MB | 64: BML / FB (s) | MB |
|---|---:|---:|---:|---:|---:|---:|
| plain backward search, move structure alone | 20.60 / 20.18 | 668.1 | 23.07 / 22.73 | 715.9 | 24.89 / 24.59 | 790.5 |
| plain backward search, move structure, almost-funnels on the text (both strands), fixed-length, windows of 25 (τ = 0) | 14.54 / 14.19 | 801.1 | 16.18 / 15.88 | 896.0 | 17.21 / 16.95 | 1,063.8 |
| plain backward search, move structure, almost-funnels on the text (both strands), fixed-length, windows of 100 (τ = 0) | 8.45 / 8.19 | 722.8 | 9.21 / 8.98 | 784.1 | 9.79 / 9.62 | 884.9 |
| plain backward search, move structure, almost-funnels on the text (both strands), variable-length, windows of 25, τ = 6 | 6.55 / 6.40 | 912.4 | 7.17 / 7.06 | 1,098.3 | 7.65 / 7.54 | 1,447.9 |

**Speedup over same-run plain search on the move structure (BML):**

| configuration | 16 | 32 | 64 |
|---|---:|---:|---:|
| windows of 25, τ = 0 | 1.42 | 1.43 | 1.45 |
| windows of 100, τ = 0 | 2.44 | 2.50 | 2.54 |
| windows of 25, τ = 6 | 3.15 | 3.22 | 3.25 |

**Steps and jumps (BML, first repeat; logs `logs/q<N>.<config>.bml.1.log`):**

| configuration | 16: char steps / jumps / chars crossed | 32 | 64 |
|---|---:|---:|---:|
| plain | 259.8 M / – / – | 285.3 M | 304.1 M |
| 25, τ = 0 | 55.5 M / 8.17 M / 204.2 M | 59.2 M / 9.04 M / 226.1 M | 62.2 M / 9.68 M / 242.0 M |
| 100, τ = 0 | 57.9 M / 2.02 M / 201.9 M | 62.0 M / 2.23 M / 223.3 M | 65.7 M / 2.38 M / 238.4 M |
| 25, τ = 6 | 36.0 M / 1.27 M / 223.8 M | 37.7 M / 1.37 M / 247.6 M | 39.3 M / 1.46 M / 264.8 M |

- **Windows of 25 vs 100 at τ = 0:** ℓ = 25 is slower than ℓ = 100, although both cross about the same number of characters, because it takes four times as many jumps. Each jump converts the cursor to rows, looks up the entrance (an Elias–Fano predecessor), makes up to three wavelet-matrix ranks, and converts the new interval back to a cursor (two predecessor searches). At 16, (14.54 − 8.45 s) / (8.17 − 2.02 M jumps) is about 1 µs per extra jump. That's a rough figure: it also counts the more frequent entrance checks. A cheaper jump (keeping rows alongside the cursor, or storing the target cursors instead of rows) would help ℓ = 25 most. Not tried.
- **The run log:** `run.log`, and per-run logs in `logs/`.
- **Self-test:** `selftest.log` (it passes). The new step builds four configurations on the self-test's collection and checks MEMs and intervals against plain search, checking every jump against the RLCSA's backward steps.

## Against Reportable Shortcuts' own runs (RLFM-index of the text and of its reverse, forward strands only; 2026-11-11, not the same day)
- **Shortcut sizes (MB):** about the same, though ours cover both strands in one index and theirs cover two indexes (text and reverse):

  | configuration | ours: 16 / 32 / 64 | theirs: 16 / 32 / 64 |
  |---|---|---|
  | windows of 25, τ = 0 | 132.9 / 180.2 / 273.2 | 134.9 / 178.3 / 263.8 |
  | windows of 100, τ = 0 | 54.7 / 68.2 / 94.4 | 59.4 / 71.2 / 94.1 |
  | windows of 25, τ = 6 | 244.2 / 382.5 / 657.4 | 239.5 / 367.1 / 620.9 |

- **Times:** theirs were 67.27 / 75.50 / 79.85 s (25, τ = 0), 71.41 / 80.12 / 86.00 s (100, τ = 0) and 39.32 / 42.77 / 46.15 s (25, τ = 6, BML). Ours are 4.6–8.8 times faster in absolute time (4.6 at 25, τ = 0; 8.5–8.8 at 100; 6.0 at τ = 6), mostly from the move structure.
- **Speedups over plain search:** ours are smaller (τ = 6: 3.2 against their 7.7–8.1), since our plain baseline is about 15 times faster than theirs (20.6 s against 305 s at 16).
