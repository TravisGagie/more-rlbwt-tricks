# Blocks with the smallest or the largest successor, chosen per phrase (2026-11-08)

2026-11-08, for More RLBWT Tricks. These are counts from the parse only; nothing is implemented in the index.

**Data.** These are the chr19 two-level indexes of the paper's Table 1, `~/research/rlbwt-tricks/chr19/c{33,65,129}.lvl` (16, 32 and 64 other individuals, with CHM13 first). They use the same parse: closed syncmers with k = 51 and s = 12, over both strands, as indexed. Their SHA-256 sums:
- c33: `203214bb…3204a4`
- c65: `3fffe327…4725`
- c129: `eadb2ce4…6475`

The text and the BWT are on the external drive, which isn't mounted, but they aren't needed. `experiments/successor_rules.cpp` recovers the parse by inverting the parse BWT stored in the `.lvl`. It then checks that its smallest successors equal the stored ones, and that its z_Δ and σ_Δ equal the stored ones.

The query haplotypes are not in the indexed collection. The request's "plus the two query haplotypes" refers to the paper's setup: the queries were searched against these indexes, not indexed.

**Rules.** χ(p) for each distinct phrase p is one of:
- **smallest:** the lexicographically smallest phrase following p (the paper's rule).
- **largest:** the largest phrase following p.
- **per phrase:** whichever of the two follows p more often, the smallest on a tie (one bit per phrase).
- **frequent:** the most frequent successor, the smallest on a tie (`--continuation frequent`).
- **unique** (added 2026-11-08 evening, request "Unitig blocks"): χ(p) is p's successor if p has exactly one distinct successor in the parse, and none otherwise, so a block ends at every phrase with two or more distinct successors. These are unitig-like blocks: the smallest rule restricted to such phrases, with no ω bits.

**Definitions.**
- "Followed": the phrase occurrences i < m − 1 with P[i+1] = χ(P[i]), out of the m − 1 occurrences that have a successor.
- z_Δ = m − followed.
- σ_Δ: the distinct block symbols (d, j, side), keyed as `twolevel_build.hpp` keys them.
- r_Δ: the runs in the BWT of the block sequence, computed with libsais in the index's symbol order (by d; − before +; −'s by j ascending, +'s by j descending).

For the largest and per-phrase rules the side is determined by (d, j) (the last block aside), so σ_Δ is the number of distinct (d, j).

## 1–2. Fraction followed by χ, z_Δ, σ_Δ, r_Δ

| individuals | rule | followed by χ | z_Δ (blocks) | σ_Δ | r_Δ |
|---|---|---:|---:|---:|---:|
| 16 | smallest | 89.50% | 23,158,046 | 1,225,789 | 2,613,788 |
| 16 | largest | 89.33% | 23,524,443 | 1,222,426 | 2,618,031 |
| 16 | per phrase | 92.23% | 17,127,050 | 1,176,695 | 2,380,038 |
| 16 | frequent | 96.73% | 7,207,544 | 1,295,517 | 2,019,545 |
| 32 | smallest | 87.97% | 52,209,743 | 1,780,430 | 4,548,667 |
| 32 | largest | 87.79% | 53,020,440 | 1,774,492 | 4,551,591 |
| 32 | per phrase | 91.67% | 36,168,623 | 1,751,008 | 4,142,168 |
| 32 | frequent | 96.68% | 14,425,400 | 1,950,966 | 3,500,457 |
| 64 | smallest | 86.01% | 120,622,565 | 2,580,091 | 8,009,328 |
| 64 | largest | 85.79% | 122,516,988 | 2,572,240 | 8,012,801 |
| 64 | per phrase | 90.98% | 77,762,910 | 2,602,881 | 7,353,671 |
| 64 | frequent | 96.63% | 29,053,987 | 2,956,029 | 6,147,538 |
| 16 | unique | 85.11% | 32,833,124 | 1,333,210 | 2,987,555 |
| 32 | unique | 82.64% | 75,378,134 | 1,849,468 | 5,017,741 |
| 64 | unique | 79.44% | 177,297,475 | 2,598,749 | 8,602,928 |

- **The paper's numbers are reproduced exactly.** The smallest rule's z_Δ, σ_Δ and r_Δ equal the paper's indexes' (`results/chr19/logs/run33.log`, `rebuild65.log`; `results/block_scaling/chr19_table.txt` and `blocks.tsv`). So do the frequent rule's (`blocks.tsv`, `bbwt` column).
- **Per phrase against smallest:** z_Δ is 26%, 31% and 36% smaller (16, 32 and 64 individuals). r_Δ is 9%, 9% and 8% smaller. σ_Δ is 4% and 2% smaller at 16 and 32, and 1% larger at 64.
- **Frequent is still far ahead:** 2.4, 2.5 and 2.7 times fewer blocks than per phrase.
- **Unique against smallest:** 42%, 44% and 47% more blocks (16, 32 and 64 individuals). σ_Δ is 9%, 4% and 1% larger and r_Δ 14%, 10% and 7% larger. The block order equals the parse order here too.
- **Block order:** `--check-order` sorted the parse's suffixes and checked, for every rule and size, that the block suffixes are in the same order as the parse suffixes at block starts. They are in all 15 cases. This confirms the ordering claim for the largest and per-phrase rules: one side per phrase, so no ω bits are needed.

## 3. Phrases with at least 3 distinct successors

| individuals | distinct phrases with ≥ 2 successors | with ≥ 3 | share of occurrences (≥ 3) | most frequent = smallest | = largest | neither | max tied |
|---|---:|---:|---:|---:|---:|---:|---:|
| 16 | 571,129 | 79,732 | 7.76% | 29.2% | 21.5% | 49.3% | 7.3% |
| 32 | 780,529 | 123,644 | 8.57% | 27.5% | 22.0% | 50.4% | 4.5% |
| 64 | 1,072,637 | 196,961 | 9.66% | 26.2% | 22.0% | 51.7% | 3.0% |

- "Most frequent" breaks ties toward the smallest, as the index does.
- Counting any successor that attains the maximum, the largest attains it in 26.4%, 25.0% and 24.1% of these phrases. The smallest's figures don't change.
- The share of occurrences is the occurrences (with a successor) of these phrases over all m − 1.

## Files

- `c33.tsv`, `c65.tsv`, `c129.tsv`: the numbers.
- `c*.log`: progress, checks, time and peak memory (2.1, 4.7 and 10.7 minutes; peak 3.0, 5.5 and 10.6 GB, with the order check and five rules).
- `run.sh`: how they were made.
- The code: `experiments/successor_rules.cpp`.
