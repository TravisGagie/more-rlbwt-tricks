# Would a third level help? chr19 at 16, 32 and 64 individuals (2026-11-09)

Would three-level indexing help? A measurement, not an implementation. The program is `experiments/three_level.cpp` (`bin/three-level`); its header describes it in full.

**What it does:**
- **Level 1:** the paper's parse, recovered from the two-level index `c<N>.lvl` (closed syncmers k = 51, s = 12 over characters). With `unique/c<N>u.lvl`, the blocks are those of the unitig rule `unique`; the parse is the same.
- **Level 2:** the parse of the parse, by closed syncmers over phrase ids, as recursive PFP parses its parse.
  - Each id gets a 64-bit hash, and each s₂-mer of ids the hash of its s₂ hashes.
  - A window of k₂ ids is a closed syncmer when its smallest s₂-mer is its first or its last.
  - A level-2 phrase runs from one trigger t_j to the next plus k₂ ids, P[t_j .. t_{j+1} + k₂). Consecutive level-2 phrases overlap by k₂ ids, as level-1 phrases overlap by k characters, so a level-2 phrase is determined by its own ids.
  - Settings: s₂ = 2 and k₂ = 5, 9, 17, 33, 65. That gives level-2 phrases of about 2, 4, 8, 16 and 32 phrases, (k₂ − s₂ + 1)/2.
- **The MEMs:** the paper's MEMs of length at least 1000 for the two query haplotypes (`blocks/q<N>s.blocks.tsv`). Each pattern is parsed as `find` parses it. Within each MEM the program counts:
  - the whole level-1 phrases inside it;
  - the block steps over them (one per block start);
  - the whole level-2 phrases inside it, with triggers taken from the windows inside the MEM only;
  - the fringes: the level-1 phrases before the first whole level-2 phrase and after the last one's core (from its t_{j+1} on, so they include the k₂-id overlap);
  - the blocks the fringes need.
- **Steps compared,** over the MEMs' whole level-1 phrases:
  - blocks alone: one step per block;
  - level 2 with level-1 steps in the fringes: level-2 phrases plus fringe phrases;
  - level 2 with block steps in the fringes: level-2 phrases plus fringe blocks.

  Characters outside whole phrases are the same for every method and aren't counted. Neither are the failed extensions BML also tries. At 16, the whole search makes 2.55 million parse steps and 1.81 million block steps (`results/chr19/unique/`), while the MEMs' whole phrases take 0.96 million block steps.
- **Check (`--check`, at 16):** all 9,002,797 whole level-2 phrases found inside MEMs are level-2 phrases of the text, by fingerprint. Every whole level-1 phrase inside a MEM is in the dictionary, at every size.

**Run** from `tool/` (one thread; peak 2.9 GB at 16, 5.4 GB at 32, 10.5 GB at 64):
```
bin/three-level ~/research/rlbwt-tricks/chr19/c33 ~/research/rlbwt-tricks/chr19/blocks/queries.fa ~/research/rlbwt-tricks/chr19/blocks/q33s.blocks.tsv --check > c33.tsv
bin/three-level ~/research/rlbwt-tricks/chr19/unique/c33u ... > c33u.tsv      (and c65, c65u, c129, c129u without --check)
```
Output: `c<N>.tsv` (blocks with the smallest successor, the paper's), `c<N>u.tsv` (`unique`), and logs.
