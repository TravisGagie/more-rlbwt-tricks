# The hash seeds as parameters (2026-11-24)

`index --hash-seed X` sets the seed of the phrase dictionary's 64-bit hash (`phrase_hash`), and `index --kr-base X` the
base of the shortcuts' Karp–Rabin fingerprints (mod 2^61 − 1). The defaults are the values every earlier index used.
A seed or base other than the default is stored in the file: the dictionary flags it in bit 31 of its bucket-bit
count, and the shortcuts file becomes `KTPSC002`. A default one is not stored, so the file format is unchanged. Readers
built from older headers can't read a stored seed (they must be rebuilt). `find --stats` now prints the dictionary
lookups and fingerprint comparisons. Driver: `experiments/textless/seed_check.sh`. No timing.

**Defaults change nothing** (`defaults.txt`), at 16 individuals:
- rebuilt with the new binary and the default seeds, every file is byte-identical to the one the paper's runs used:
  `c33.lvl`, `c33.rlbwt`, ▷'s `c33u.lvl`, the text-free `c33.tlx`, `c33v.tl2`, `c33uv.tl2`, and heavy paths θ = 80%'s
  `c33.heavy08.psc`;
- the new binary on the paper's files gives byte-identical output to the final session's, rows `tl`, `tlur`, `tlh08`,
  BML and FB.

**Other seeds work** (`seeded.txt`), at 16 individuals, `--hash-seed 0x0123456789abcdef`, `--kr-base 0x1d2c3b4a5968778`:
- `c33s.lvl` is 8 bytes longer than `c33.lvl` (the stored seed) and differs in the dictionary; the `.tl2` files are
  byte-identical to the default ones (they don't depend on the hash); the `.psc` differs in its fingerprints;
- plain two-level search, ▷ with the jump and heavy paths θ = 80%, without the index of the text, BML and FB: the MEMs
  and occurrence counts equal plain backward search's, and every search equals two-level search's on the index of the
  text (0 differences in j and in occurrences).

**Counts for a run of all the queries** (`counts.txt`; the same queries at every size), 16 / 32 / 64:

| | 16 | 32 | 64 |
|---|---:|---:|---:|
| dictionary lookups | 6,650,901 | 6,650,901 | 6,650,901 |
| — not phrases of the text (found nowhere) | 35,634 | 19,454 | 14,318 |
| fingerprint comparisons, heavy paths θ = 80%, BML / FB | 1,079,224 / 1,089,882 | 1,105,914 / 1,116,512 | 1,137,338 / 1,144,183 |
| — identifiers compared in them, BML / FB | 25.5 / 27.8 M | 25.5 / 27.4 M | 25.8 / 27.5 M |
| fingerprint comparisons, θ = 90%, BML / FB | 888,085 / 895,026 | 920,053 / 930,255 | 966,767 / 978,391 |
| fingerprint comparisons, θ = 95%, BML / FB | 815,789 / 830,751 | 885,987 / 906,881 | 960,187 / 982,378 |
