# Remaining reruns with the fixed binary, chr19 (2026-11-12, 18:53–19:38)

*Commit hashes in this README refer to the private development history of this code; those commits are not in this repository.*

Reruns with the fixed binary (2026-11-12). `bin/katka` at 5c576a8 (5a59c62's A–C
fixes, plus `--move` for plain backward search). Script: `O=results/chr19/reruns_fixed experiments/twolevel_ac.sh
"33 65 129" "<configurations>"`; table `table.md` by `experiments/twolevel_ac_table.py`; logs in `logs/`.
L = 1000, the two query haplotypes, single thread pinned to CPU 0, 11 GB cgroups, 3 interleaved rounds per size;
medians of 3. Every run's MEMs equal the paper's BML output (columns 1–3), and so do the intervals (columns 4–5) in
all configurations but the STPD of the parse, which reports one occurrence per MEM and no interval.

- **MB loaded:** the files `find` opens, as in Table 1. Plain RLCSA search opens only `c<N>.csa` (and the 17-byte
  `.info`), since 84ebe46 (2026-11-06: "find loads the RLBWT only where it is needed"; checked with strace, today's
  binary and the one before today's changes); so 579.3 / 1,192.7 / 1,301.4 MB, not Table 1's 771 / 1,410 / 1,555,
  which add the RLFM-index. With `--move` alone it opens only `c<N>.mv` (+ the 2.1 MB 8-mer table built at load).
  Two-level: `.rlbwt` + `.csa` or `.mv` + `.lvl` + the shortcuts. STPD of the parse: `.rlbwt`, `.csa`, `.lvl`,
  `.pstpd`, `.ptxt`, `.stpd`.
- **The move structure in plain search:** every step of plain BML and FB at 16 (259,771,898 and 250,779,805) was
  checked against the RLCSA's (`KATKA_MOVE_CHECK=1`); all agree.
- **STPD of the parse:** `--move` does not apply (its right end steps on the RLCSA in other code, and its left end is
  the character-level STPD), so only the RLCSA row.

| configuration | 16: BML / FB (s) | MB | 32: BML / FB (s) | MB | 64: BML / FB (s) | MB |
|---|---:|---:|---:|---:|---:|---:|
| plain backward search, RLCSA | 43.32 / 42.02 | 579.3 | 53.79 / 53.04 | 1,192.7 | 55.44 / 54.86 | 1,301.4 |
| plain backward search, move structure alone | 20.26 / 19.80 | 668.1 | 22.65 / 22.32 | 715.9 | 23.68 / 23.44 | 790.5 |
| two-level, almost-funnels from CHM13's parse, g = 4; RLCSA | 6.50 / 6.64 | 997.8 | 6.76 / 6.82 | 1,682.4 | 7.08 / 7.09 | 1,901.8 |
| two-level, almost-funnels from CHM13's parse, g = 4; move structure | 5.99 / 6.06 | 1,086.6 | 6.28 / 6.29 | 1,205.6 | 6.61 / 6.60 | 1,391.0 |
| two-level, almost-funnels from CHM13's parse, g = 16; RLCSA | 6.57 / 6.62 | 986.0 | 6.84 / 6.83 | 1,669.0 | 7.13 / 7.10 | 1,885.4 |
| two-level, almost-funnels from CHM13's parse, g = 16; move structure | 6.03 / 6.01 | 1,074.8 | 6.38 / 6.30 | 1,192.2 | 6.70 / 6.57 | 1,374.6 |
| two-level, almost-funnels from CHM13's parse, g = 64; RLCSA | 6.92 / 6.83 | 982.2 | 7.28 / 7.09 | 1,664.1 | 7.61 / 7.46 | 1,878.2 |
| two-level, almost-funnels from CHM13's parse, g = 64; move structure | 6.41 / 6.22 | 1,071.0 | 6.76 / 6.53 | 1,187.3 | 7.12 / 6.92 | 1,367.4 |
| two-level, heavy paths θ = 0.9; RLCSA | 3.86 / 3.90 | 1,118.4 | 3.78 / 3.75 | 1,815.8 | 3.76 / 3.68 | 2,056.0 |
| two-level, heavy paths θ = 0.9; move structure | 3.41 / 3.38 | 1,207.2 | 3.34 / 3.26 | 1,339.0 | 3.34 / 3.22 | 1,545.2 |
| two-level, heavy paths θ = 0.95; RLCSA | 3.89 / 3.93 | 1,110.6 | 3.85 / 3.83 | 1,807.2 | 3.82 / 3.78 | 2,041.9 |
| two-level, heavy paths θ = 0.95; move structure | 3.44 / 3.42 | 1,199.4 | 3.39 / 3.31 | 1,330.4 | 3.43 / 3.31 | 1,531.1 |
| our STPD of the parse (plain parse), one occurrence per MEM; RLCSA | 4.85 / 5.47 | 2,660.0 | 5.05 / 5.53 | 4,273.5 | 5.25 / 5.74 | 6,302.8 |
