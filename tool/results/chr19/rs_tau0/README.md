# Reportable Shortcuts with fixed-length almost-funnels (τ = 0), chr19 (2026-11-11)

Reportable Shortcuts' own code (`shortcuts/` at the top of this repository, `build_index` and `query_mems`), with CHM13 as
the reference, maximum tier τ = 0 (every shortcut crosses one window), windows of ℓ = 25 and ℓ = 100, θ = 0. The
index is theirs: the RLBWTs of the text and of its reverse, forward strands only. Script `experiments/rs_tau0.sh`.

- **Search:** `query_mems --algorithm bml,fb --min-length 1000 --repeat 3` on the two query haplotypes, pinned to one
  core, in an 11 GB cgroup. Each round runs plain search (no shortcuts) and then each set of shortcuts, so the plain
  times are from the same run; medians of 3, loading excluded. `query_mems` stops on any MEM that differs from plain
  search's; none did: 22,708 / 18,479 / 16,804 MEMs (one strand; ours over both strands find 22,711 / 18,479 / 16,806).

| configuration | 16: BML / FB (s) | 32 | 64 | MB (RLBWTs + shortcuts), 16 / 32 / 64 |
|---|---|---|---|---|
| plain | 304.65 / 300.58 | 342.10 / 332.72 | 364.64 / 360.19 | 329.5 / 365.8 / 425.4 |
| ℓ = 25, τ = 0 | 67.27 / 69.19 | 75.50 / 74.92 | 79.85 / 80.75 | 464.4 / 544.1 / 689.1 |
| ℓ = 100, τ = 0 | 71.41 / 71.99 | 80.12 / 78.91 | 86.00 / 84.90 | 388.9 / 437.0 / 519.5 |

- **Files:** `summary.txt`, `n<size>.{csv,log}` (per pattern and round), and the builds `build<size>.{fwd,rev}.log`
  (`build_index --max-tier 0 --theta 0 --ell 25,100`, 171–194 s per direction, peak 2.3 / 3.9 / 7.2 GB).
- Our almost-funnels on the text over one index of both strands, on the move structure, are in `../tsc_text/`.
