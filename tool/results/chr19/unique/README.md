# Unitig blocks (`--continuation unique`) against the smallest successor, chr19 (2026-11-08, evening)

The counts are in `../successor_rules/`.

**The rule:** a phrase continues a block only if it has exactly one distinct successor in the parse. All symbols are +, and there are no ω bits.
- **Build:** `katka index --twolevel --continuation unique`.
- **Search:** unchanged. A phrase with two or more successors has χ = 0, so a block always ends after it, in the text as in the pattern.

**The indexes:** `~/research/rlbwt-tricks/chr19/unique/c{33,65,129}u.lvl` (checksums in `lvl.sha256`).
- They were built with `katka index --twolevel --continuation unique --blocks-from ~/research/rlbwt-tricks/chr19/c<n>.lvl c<n>u` (new: it rebuilds only the blocks, from the parse recovered by inverting the stored parse BWT, so no text or BWT is read).
- Their parse, B, parse BWT and dictionary are the paper's. Their RLBWT and RLCSA are links to the paper's.
- **Check of `--blocks-from`:** with `--continuation smallest` it rebuilt `c33.lvl` byte-identical to the paper's, and with `--continuation frequent` byte-identical to `blocks/c33f.lvl`.
- Build logs: `logs/index*u.log` (65, 134 and 301 s; peak 2.2, 4.1 and 8.0 GB).

**Searches:** as in Table 1.
- **Queries:** the two query haplotypes (`blocks/queries.fa`, 122,434,624 characters in 4 patterns).
- **Command:** `find -L 1000 --format mems --csa --twolevel --stats -j 1`.
- **Conditions:** pinned to CPU 0, in an 11 GB cgroup without swap, 3 repeats.
- **Time:** the tool's own search time, which excludes loading.
- **Correctness:** every run's output, MEMs and their BWT intervals, is byte-identical to the paper's `blocks/q<n>s.blocks.tsv`, which equals plain BML's.
- Script: `experiments/unique_timing.sh`; numbers: `runs.tsv`, `table.md`; logs: `logs/`.

| individuals | index | .lvl MB | time s (median of 3) | parse steps | block steps | entry | inside | end | block attempts | failed | back-ups |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 16 | no block steps | 208.5 | 8.66 | 15,123,381 | 0 | | | | | | |
| 16 | smallest | 208.5 | 5.89 | 2,547,359 | 1,814,194 | 1,333,558 | 61,513 | 1,152,288 | 147,083 | 72,088 | 25,207 |
| 16 | unique | 210.3 | 5.67 | 1,571,797 | 2,495,942 | 725,242 | 43,785 | 802,770 | 159,066 | 83,341 | 25,013 |
| 32 | no block steps | 249.9 | 8.79 | 16,368,497 | 0 | | | | | | |
| 32 | smallest | 249.9 | 5.54 | 1,569,397 | 2,130,105 | 764,825 | 55,016 | 749,556 | 123,828 | 67,468 | 23,861 |
| 32 | unique | 252.4 | 5.66 | 917,553 | 3,019,407 | 387,273 | 39,475 | 490,805 | 128,387 | 72,242 | 23,798 |
| 64 | no block steps | 318.0 | 9.55 | 17,344,241 | 0 | | | | | | |
| 64 | smallest | 318.0 | 5.69 | 1,107,211 | 2,548,855 | 516,634 | 54,512 | 536,065 | 114,329 | 65,460 | 23,887 |
| 64 | unique | 321.1 | 5.92 | 629,536 | 3,703,569 | 251,605 | 38,211 | 339,720 | 115,661 | 68,004 | 23,730 |

The character steps are the same in every configuration of a size: 6,802,862, 5,329,354 and 4,752,917. `table.md` has every column and the three times of each run.

- **Speed: about the same.** Unique against smallest is −3.7%, +2.2% and +4.0% (16, 32, 64). The runs of a configuration are within 0.1 s of each other, except smallest at 16 (5.88–6.05). The paper's single runs were 5.66, 5.54 and 5.65 (Table 1). The no-block-steps rows (8.66, 8.79, 9.55) match the paper's 8.46, 8.95 and 9.45 within run-to-run noise.
- **Why it's a wash:** unique blocks are shorter, 6.71, 5.76 and 4.86 phrases against 9.52, 8.32 and 7.15. So the search takes 38–43% fewer parse steps (fewer entry and end steps), but 38–45% more block steps. Parse and block steps cost about the same, and their sum is about the same: 4.07M against 4.36M at 16, 3.94M against 3.70M at 32, and 4.33M against 3.66M at 64.
- **Size:** the `.lvl` is 1.8, 2.5 and 3.1 MB larger (more blocks, so a larger BS and block BWT).
