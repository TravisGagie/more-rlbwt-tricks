# Costs shared by the two-level methods: fringe steps, pattern parsing, input (2026-11-12)

Costs shared by the two-level methods (2026-11-12), A–C and the same-day reruns. chr19, the two query
haplotypes (122.4 Mbp), L = 1000, single thread pinned to CPU 0, 11 GB cgroups.

- **Profiles:** `experiments/twolevel_profile.sh` (`bin/katka-prof`, `-DKATKA_PROF`, `src/prof.hpp`), logs in
  `results/chr19/twolevel_profile/logs/`. The `*.prof.log` files are from before the fixes, and the `*.after-fixes.*`
  files from after them. The unprofiled before-the-fixes log of ▷ BML was overwritten by the after-fixes rerun
  (now `q33.unique.bml.plain.after-fixes.log`); the profiled one is intact.
- **Reruns:** `experiments/twolevel_ac.sh` (logs in `logs/`), table `table.md` by `experiments/twolevel_ac_table.py`.

## A. Fringe character steps (16 individuals, ▷, BML)

| | backward searches | forward searches |
|---|---:|---:|
| searches / reaching the parse | 70,246 / 64,144 | 22,711 / 22,711 |
| right-end steps (per search) | 4,264,011 (60.7) | 1,421,723 (62.6) |
| left-end steps (per search reaching the parse) | 831,199 (13.0) | 285,929 (12.6) |

The right end is the window's last characters, back to its last syncmer start: at least k = 51 of them. The steps are
the same in every two-level configuration.

| text index | right end: s (ns per step) | left end: s (ns per step) | MB at 16 / 32 / 64 |
|---|---|---|---|
| RLCSA (`--csa`) | 0.837 (147) | 0.173 (155) | 579.3 / 1,192.7 / 1,301.4 |
| our RLFM-index (no `--csa`) | 3.146 (553) | 0.609 (545) | 191.3 / 217.6 / 253.6 |
| **move structure (`--move`)** | **0.444 (78)** | **0.135 (121)** | **676.7 / 724.9 / 800.5** |

- **Why the left end cost more in our STPD of the parse:** there it is the character-level STPD, through the text
  oracle: 2.04 s for 1.11 M steps, about 1.8 µs per step. In two-level search both ends use the same text index.
- **The move structure** (`src/move.hpp`, `index --move` builds `PREFIX.mv` from the `.rlbwt` in 9 s, peak 2.6–3.0
  GB): one 64-bit record per run (symbol, length, LF of its first row as run and offset), runs split at 65,535
  (68,446,512 / 72,587,408 / 79,393,831 runs, 27–226 more than before splitting); the run starts (Elias–Fano), to
  convert at the level changes; and per symbol the indices of its runs, for repositioning an end when a scan of 8
  neighbouring runs fails. The interval is kept as (run, offset) at both ends. At load it builds the cursors of all
  8-mers (2.1 MB), which replace the first 8 steps from the full interval (t = 12: 537 MB for 0.03 s more).
- **Checks:** with `KATKA_MOVE_CHECK=1` and `--csa`, every step is compared with the RLCSA's: all 6,802,862 at 16
  agree. The selftest does the same on its chromosome-like collection (BML and FB, with and without jumps, with
  shortcuts), and every rerun's MEMs and intervals equal the paper's.
- **Entering the parse:** the two ranks of B now share their predecessor search (`RLBits::rank1_pair`): 0.131 → 0.082 s.

## B. Pattern parsing (16, ▷, BML; s)

| part | before | after | what changed |
|---|---:|---:|---|
| closed-syncmer scan | 0.702 | 0.722 | nothing (see below) |
| phrase hashes | 0.242 | 0.239 | nothing |
| dictionary lookups and ids | 0.405 | 0.244 | each slot's hash bits and both ids in one packed record (built at load; the file is unchanged); bucket starts prefetched 64 phrases ahead and records 32 ahead |
| block marks (`finish`) | 0.286 | 0.137 | successor lookups prefetched 16 phrases ahead |
| **total** | **1.634** | **1.343** | |

- **The scan** is already a sliding-window minimum (van Herk / Gil–Werman, in chunks): 3.9 ns per base on its own,
  2.2 of them computing the s-mers' `mix64` hashes, which must stay as they are to give the index's syncmers. Smaller
  chunks (to stay in L1) and a separate hashing pass made no difference or were slower.

## C. "Everything else" (16, ▷, BML; s)

| part | before | after | what changed |
|---|---:|---:|---|
| reading the FASTA | 0.417 | 0.151 | lines appended to the sequence as read (not via a second string); upper-casing by table |
| reverse complement of each read | 0.773 | 0.133 | a table instead of `complement()`'s switch, which mispredicts on DNA |
| the rest (BML's bookkeeping, segments, output) | 0.163 | 0.163 | |

## Totals (16, ▷, BML, profiled)

5.85 s before → 4.62 s with B and C (RLCSA) → **4.14 s** with A too (move structure).

## Same-day reruns with the fixed binary (medians of 3, s; MB loaded)

See `table.md`. MB: the files `find` opens, as in Table 1 (plain RLCSA: `.csa`; two-level: `.rlbwt` + `.csa` or
`.mv` + 8-mer table + `.lvl` (+ shortcuts)).
| configuration | 16: BML / FB (s) | MB | 32: BML / FB (s) | MB | 64: BML / FB (s) | MB |
|---|---:|---:|---:|---:|---:|---:|
| plain backward search, RLCSA | 41.77 / 40.43 | 579.3 | 52.73 / 51.84 | 1,192.7 | 57.70 / 57.04 | 1,301.4 |
| two-level, no jumps; RLCSA | 7.05 / 6.98 | 979.1 | 7.47 / 7.34 | 1,660.2 | 8.26 / 8.09 | 1,872.9 |
| two-level, no jumps; move structure | 6.56 / 6.39 | 1,067.9 | 6.96 / 6.77 | 1,183.4 | 7.75 / 7.61 | 1,362.1 |
| two-level, ▷; RLCSA | 4.38 / 4.41 | 980.9 | 4.38 / 4.36 | 1,662.7 | 4.67 / 4.59 | 1,876.0 |
| two-level, ▷; move structure | 3.91 / 3.87 | 1,069.7 | 3.94 / 3.84 | 1,185.9 | 4.22 / 4.11 | 1,365.2 |
| two-level, heavy paths θ = 0.8; RLCSA | 3.88 / 3.87 | 1,127.9 | 3.77 / 3.70 | 1,832.8 | 3.77 / 3.69 | 2,084.8 |
| two-level, heavy paths θ = 0.8; move structure | 3.43 / 3.38 | 1,216.7 | 3.33 / 3.25 | 1,355.9 | 3.36 / 3.25 | 1,574.0 |
