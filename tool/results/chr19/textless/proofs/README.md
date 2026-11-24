# The built text-free index by component, and facts about its phrases (2026-11-24)

*Commit hashes in this README refer to the private development history of this code; those commits are not in this repository.*

`bin/textless-components ~/diamond/c<N>tl ~/diamond/c<N>.tlx > components.c<N>.tsv`
(`experiments/textless/components.cpp`), on the minimal-width index of part C1 (`c<N>tl.lvl`, `c<N>tl.tl2`). No search
is run. Sizes in bytes, as loaded in memory, without the first-run table; the files are 0.8% smaller in all
(313.3 / 369.5 / 470.4 MB).

**Space by component (MB = 10^6 bytes), 16 / 32 / 64 individuals:**

| component | 16 | 32 | 64 |
|---|---:|---:|---:|
| the parse's BWT (`parse_bwt`) | 49.7 | 64.5 | 90.0 |
| the dictionary (`dictionary`) | 105.2 | 116.7 | 133.6 |
| the two short copies, bases (`copies_bases`) | 82.2 | 94.2 | 115.4 |
| — their offsets (`copies_offsets`) | 8.0 | 9.0 | 10.6 |
| the phrases with X or $, whole (`whole_phrases`) | 3.6 | 6.6 | 17.7 |
| the ranges: a bit per phrase (`ranges_bits`) | 1.6 | 1.8 | 2.1 |
| — a pointer per phrase to its last k-mer's range (`ranges_lastr`) | 24.5 | 27.4 | 33.0 |
| the run heads: starts (`run_heads_starts`) | 10.0 | 13.5 | 19.6 |
| — symbols (`run_heads_symbols`) | 31.0 | 38.9 | 52.3 |
| **total** | **315.8** | **372.5** | **474.3** |

**Phrases:**
- The longest phrase over A/C/G/T has 90 characters in all three collections (2k − s with k = 51, s = 12).
- 89 / 177 / 328 phrases contain X or $; the longest has 419,845 / 1,158,897 / 2,137,410 characters.
- Every phrase over A/C/G/T is shorter than 2k = 102, so neither short copy holds its characters [|y| − k, k): 271.8 /
  302.1 / 346.4 million bases in all (`acgt_bases_in_neither_copy`), 67.9 / 75.5 / 86.6 MB at 2 bits per base.
- The text's first phrase (the row preceded by the parse's sentinel) has 80 characters over A/C/G/T, does not start
  with a closed syncmer, and is alone in its range.

**Ranges:** the largest range of identifiers (phrases with the same first k-mer) has 2,092 / 2,800 / 3,894 phrases
(`largest_range`). That bounds the phrases one binary search at the right end can cover, in every configuration: they
all have the same phrases.
