| individuals, L | MEMs | no parse level | right end only | with parse steps | sum of occ | sum of survivors | survivors = occ | survivors / sum occ |
|---|---|---|---|---|---|---|---|---|
| 16, 1000 | 22,711 | 0 | 0 | 22,711 | 72,876 (72,876) | 94,569 | 22,140 | 1.298 |
| 16, 100 | 38,984 | 0 | 57 | 38,927 | 384,341 (383,007) | 8,958,153 | 35,004 | 23.389 |
| 32, 1000 | 18,479 | 0 | 0 | 18,479 | 64,945 (64,945) | 102,179 | 17,933 | 1.573 |
| 32, 100 | 30,809 | 0 | 39 | 30,770 | 362,636 (360,561) | 13,794,404 | 27,181 | 38.258 |
| 64, 1000 | 16,806 | 0 | 0 | 16,806 | 75,563 (75,563) | 162,410 | 16,272 | 2.149 |
| 64, 100 | 27,137 | 0 | 34 | 27,103 | 393,185 (391,281) | 17,752,684 | 23,917 | 45.371 |

**occ**, over the MEMs with parse steps:

| individuals, L | mean | median | 90% | 99% | maximum |
|---|---|---|---|---|---|
| 16, 1000 | 3.21 | 1 | 8 | 24 | 166 |
| 16, 100 | 9.84 | 2 | 16 | 98 | 10,076 |
| 32, 1000 | 3.51 | 1 | 8 | 34 | 363 |
| 32, 100 | 11.72 | 1 | 17 | 164 | 14,923 |
| 64, 1000 | 4.50 | 1 | 9 | 55 | 785 |
| 64, 100 | 14.44 | 1 | 20 | 219 | 6,370 |

**survivors**, over the MEMs with parse steps:

| individuals, L | mean | median | 90% | 99% | maximum |
|---|---|---|---|---|---|
| 16, 1000 | 4.16 | 1 | 8 | 25 | 11,243 |
| 16, 100 | 230.13 | 2 | 22 | 2,114 | 175,040 |
| 32, 1000 | 5.53 | 1 | 8 | 36 | 33,632 |
| 32, 100 | 448.31 | 2 | 28 | 4,623 | 325,576 |
| 64, 1000 | 9.66 | 1 | 10 | 58 | 81,651 |
| 64, 100 | 655.01 | 2 | 34 | 5,307 | 637,584 |

**survivors / occ**, over the MEMs with parse steps:

| individuals, L | mean | median | 90% | 99% | maximum |
|---|---|---|---|---|---|
| 16, 1000 | 1.58 | 1 | 1 | 2.88 | 7.76e+03 |
| 16, 100 | 58.00 | 1 | 1.01 | 641 | 5.37e+04 |
| 32, 1000 | 2.94 | 1 | 1 | 3 | 3.36e+04 |
| 32, 100 | 129.76 | 1 | 1.09 | 1.37e+03 | 2.14e+05 |
| 64, 1000 | 1.85 | 1 | 1 | 3.15 | 1.17e+04 |
| 64, 100 | 179.26 | 1 | 1.07 | 1.76e+03 | 4.41e+05 |

**survivors - occ (the almost-MEMs)**, over the MEMs with parse steps:

| individuals, L | mean | median | 90% | 99% | maximum |
|---|---|---|---|---|---|
| 16, 1000 | 0.96 | 0 | 0 | 3 | 11,240 |
| 16, 100 | 220.29 | 0 | 1 | 1,963 | 175,025 |
| 32, 1000 | 2.01 | 0 | 0 | 3 | 33,631 |
| 32, 100 | 436.59 | 0 | 1 | 4,493 | 325,559 |
| 64, 1000 | 5.17 | 0 | 0 | 5 | 81,644 |
| 64, 100 | 640.57 | 0 | 1 | 5,306 | 637,553 |

Block rows and the left piece (MEMs with parse steps):

| individuals, L | sum of block rows among the survivors | survivors that are not block rows (MEMs, rows) | left piece: phrases of an occurrence, mean / max | stopped at pid = 0 | left piece in characters, mean / max | phrases crossed, mean / min |
|---|---|---|---|---|---|---|
| 16, 1000 | 33,147 | 18,371, 61,422 | 1.000 / 1 | 0 | 11.6 / 38 | 423.8 / 37 |
| 16, 100 | 8,183,920 | 28,259, 774,233 | 1.000 / 1 | 0 | 12.5 / 38 | 255.3 / 1 |
| 32, 1000 | 54,226 | 14,102, 47,953 | 1.000 / 1 | 0 | 11.5 / 38 | 572.0 / 40 |
| 32, 100 | 12,918,655 | 20,126, 875,749 | 1.000 / 1 | 0 | 12.6 / 38 | 350.6 / 1 |
| 64, 1000 | 111,236 | 12,371, 51,174 | 1.000 / 1 | 0 | 11.5 / 38 | 665.4 / 40 |
| 64, 100 | 16,843,547 | 16,907, 909,137 | 1.000 / 1 | 0 | 12.6 / 38 | 419.4 / 1 |

▷ and heavy paths theta = 80% against two-level search with blocks, 16 individuals, MEM by MEM:

- L = 1000, unique: 22,711 MEMs, kind, survivors or left piece differ in 0; sum of survivors 94,569
- L = 1000, heavy08: 22,711 MEMs, kind, survivors or left piece differ in 0; sum of survivors 94,569
- L = 100, unique: 38,984 MEMs, kind, survivors or left piece differ in 0; sum of survivors 8,958,153
- L = 100, heavy08: 38,984 MEMs, kind, survivors or left piece differ in 0; sum of survivors 8,958,153

- 16, 100: MEMs without parse steps: 57, lengths 100-117, occurrences 1,334; right end only: survivors 209,100 for occ 1,334
- 32, 100: MEMs without parse steps: 39, lengths 100-115, occurrences 2,075; right end only: survivors 403,209 for occ 2,075
- 64, 100: MEMs without parse steps: 34, lengths 100-115, occurrences 1,904; right end only: survivors 733,079 for occ 1,904
