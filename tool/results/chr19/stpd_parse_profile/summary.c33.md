# Profile of our STPD of the parse, c33 (16 individuals if 33), BML, L = 1000, one occurrence per MEM

Made by `experiments/stpd_parse_profile.sh 33`; logs in `logs/`. Times are medians of 3 runs.

## Time

- **Unprofiled (`bin/katka`):** 6.08 s (6.08, 6.09, 6.08)
- **Profiled (`bin/katka-prof`, TSC timers):** 6.13 s (6.13, 6.11, 6.13); the timers add 0.8%

| part | s | share of the profiled total |
|---|---:|---:|
| right end (character steps) | 0.819 | 13.4% |
| entering the parse | 0.087 | 1.4% |
| binary search | 0.365 | 6.0% |
| extension (parse loop without binary search) | 0.097 | 1.6% |
| left end (character-level STPD) | 2.039 | 33.3% |
| leaving / re-entering the parse | 0.013 | 0.2% |
| parsing the patterns (syncmers, dictionary lookups) | 1.364 | 22.3% |
| everything else (reading the reads, BML's bookkeeping, output, ...) | 1.345 | 21.9% |
| total | 6.129 | 100% |

`STPD over the parse: right-end character steps 4235527, middle phrase comparisons 13936110, parse-sample binary searches 157005 (3806497 comparisons), left-end character steps 1114294 (character-level STPD jumps 65987)`

`profile: TSC 3.192 GHz; STPD searches 47535 backward + 22711 forward; binary searches 157005, probes 3806497 (phrase comparisons 34174670); extension runs 192124, phrases read 13936110`

## LCE queries

All of them are recorded (3,998,621: 3,806,497 probes, 192,124 extension runs) in `$HOME/research/rlbwt-tricks/chr19/prof/c33.queries` (48-byte records, `src/prof.hpp`), with the patterns' phrase ids in `$HOME/research/rlbwt-tricks/chr19/prof/c33.patterns`.

- **STPD searches:** 70,246; with at least one binary search: 56,836; binary searches: 157,005.
- **Probes per search:** mean 54.19, median 48, 90% 121, 99% 217, max 524; per binary search: mean 24.24, median 24, 90% 25, 99% 25, max 25.
- **Probes, as forward LCEs** (the pattern's phrases from p against Z from x, left to right; lexicographic sample):
  - LCE length (answer): mean 8.88, median 0, 90% 9, 99% 187, max 3653;
  - phrases compared (answer - skip, plus the mismatch): mean 8.98, median 1, 90% 6, 99% 182, max 3654;
  - skipped (min of the LCPs with both ends): mean 0.90, median 0, 90% 1, 99% 16, max 1165;
  - answer = limit (the probe goes on to the next phrase's characters): 11.9%.
- **Extension, as backward LCEs** (Z[u-1] against the pattern's previous phrase, right to left):
  - per run: mean 71.72, median 16, 90% 165, 99% 956, max 6500; ended by a mismatch 81.7%;
  - phrases read per search (matches, plus each mismatch): mean 198.39, median 56, 90% 582, 99% 1803, max 7185.
