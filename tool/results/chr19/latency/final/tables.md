# Final timing session: every timed table of v3, without and with huge pages

Seconds, BML / forward–backward, median of 3 (†: median of 5, see the per-run times); MB of the files each search loads (10^6 bytes; the paper's MB column), with 16 / 32 / 64 individuals. ∗ Movi 2's own query for all MEMs. New rows are marked (new); their files are their base row's, and the first-run table they build at load is in "Sizes" below.

## Table 1, without huge pages

| | MB | 16 | 32 | 64 |
|---|---|---:|---:|---:|
| RLCSA | 579 / 1,193 / 1,301 | 39.87† / 38.37† | 48.48 / 47.53 | 53.09 / 52.44 |
| our move structure | 668 / 716 / 791 | 19.25 / 18.79 | 21.37 / 21.07 | 22.98 / 22.74 |
| ropebwt3 | 165 / 187 / 223 | 65.44 / 63.84 | 73.89 / 69.84 | 80.70 / 77.60 |
| Movi 2 | 581 / 615 / 671 | 26.25† / 57.72∗ | 30.58 / 65.21∗ | 35.90 / 84.91∗ |

## Table 1, with huge pages (GLIBC_TUNABLES=glibc.malloc.hugetlb=1)

| | MB | 16 | 32 | 64 |
|---|---|---:|---:|---:|
| RLCSA | 579 / 1,193 / 1,301 | 33.30 / 32.21 | 39.26 / 38.44 | 42.63 / 42.01 |
| our move structure | 668 / 716 / 791 | 15.99 / 15.62 | 17.70 / 17.45 | 18.98 / 18.77 |
| ropebwt3 | 165 / 187 / 223 | 59.94 / 60.38 | 67.72 / 66.10 | 73.80 / 73.81 |
| Movi 2 | 581 / 615 / 671 | 23.26 / 55.23∗ | 27.13 / 62.41∗ | 32.07 / 81.81∗ |

## Table 2, without huge pages

| | MB | 16 | 32 | 64 |
|---|---|---:|---:|---:|
| two-level, RLCSA | 979 / 1,660 / 1,873 | 6.57 / 6.49 | 6.90 / 6.79 | 7.59 / 7.51 |
| two-level, move structure | 1,068 / 1,183 / 1,362 | 6.03† / 5.89† | 6.43 / 6.30 | 7.15† / 6.98 |
| two-level, move structure, with the first-run table (new) | 1,068 / 1,183 / 1,362 | 5.47 / 5.35 | 5.74 / 5.60 | 6.36 / 6.21 |
| two-level, move structure, with the table and lookahead (d = 2) (new) | 1,068 / 1,183 / 1,362 | 4.29 / 4.26 | 4.45 / 4.36 | 4.83 / 4.71 |
| two-level, move structure, with lookahead alone (d = 4) (new) | 1,068 / 1,183 / 1,362 | 5.75† / 5.64† | 6.07 / 5.89 | 6.74 / 6.58 |

## Table 2, with huge pages (GLIBC_TUNABLES=glibc.malloc.hugetlb=1)

| | MB | 16 | 32 | 64 |
|---|---|---:|---:|---:|
| two-level, RLCSA | 979 / 1,660 / 1,873 | 5.96 / 5.91 | 6.35 / 6.17 | 6.97 / 6.81 |
| two-level, move structure | 1,068 / 1,183 / 1,362 | 5.53 / 5.37 | 5.87 / 5.70 | 6.48 / 6.31 |
| two-level, move structure, with the first-run table (new) | 1,068 / 1,183 / 1,362 | 4.94 / 4.81 | 5.20 / 5.08 | 5.74 / 5.60 |
| two-level, move structure, with the table and lookahead (d = 2) (new) | 1,068 / 1,183 / 1,362 | 3.91 / 3.87 | 4.08 / 3.98 | 4.39 / 4.29 |
| two-level, move structure, with lookahead alone (d = 4) (new) | 1,068 / 1,183 / 1,362 | 5.33 / 5.12 | 5.54 / 5.40 | 6.17 / 5.96 |

## Table 3, without huge pages

| | MB | 16 | 32 | 64 |
|---|---|---:|---:|---:|
| STPD-index | 264 / 314 / — | 39.07 / 59.43 | 44.08 / 50.98 | — |

## Table 3, with huge pages (GLIBC_TUNABLES=glibc.malloc.hugetlb=1)

| | MB | 16 | 32 | 64 |
|---|---|---:|---:|---:|
| STPD-index | 264 / 314 / — | 38.64 / 58.49 | 43.58 / 50.21 | — |

## Table 4, without huge pages

| | MB | 16 | 32 | 64 |
|---|---|---:|---:|---:|
| STPD of the parse | 2,660 / 4,273 / 6,303 | 4.55 / 5.12† | 4.74 / 5.25 | 5.08 / 5.50 |

## Table 4, with huge pages (GLIBC_TUNABLES=glibc.malloc.hugetlb=1)

| | MB | 16 | 32 | 64 |
|---|---|---:|---:|---:|
| STPD of the parse | 2,660 / 4,273 / 6,303 | 4.01 / 4.45 | 4.21 / 4.55 | 4.49 / 4.77 |

## Table 5, without huge pages

| | MB | 16 | 32 | 64 |
|---|---|---:|---:|---:|
| our move structure | 668 / 716 / 791 | 19.25 / 18.79 | 21.37 / 21.07 | 22.98 / 22.74 |
| almost-funnels, windows of 25 | 801 / 896 / 1,064 | 13.31 / 12.97 | 14.62 / 14.45 | 15.59 / 15.44 |
| almost-funnels, windows of 100 | 723 / 784 / 885 | 7.73 / 7.47 | 8.37 / 8.17 | 8.90 / 8.74 |

## Table 5, with huge pages (GLIBC_TUNABLES=glibc.malloc.hugetlb=1)

| | MB | 16 | 32 | 64 |
|---|---|---:|---:|---:|
| our move structure | 668 / 716 / 791 | 15.99 / 15.62 | 17.70 / 17.45 | 18.98 / 18.77 |
| almost-funnels, windows of 25 | 801 / 896 / 1,064 | 11.27 / 11.03 | 12.28 / 12.12 | 13.10 / 12.97 |
| almost-funnels, windows of 100 | 723 / 784 / 885 | 6.52 / 6.34 | 7.03 / 6.89 | 7.44 / 7.30 |

## Table 6, without huge pages

| | MB | 16 | 32 | 64 |
|---|---|---:|---:|---:|
| our move structure | 668 / 716 / 791 | 19.25 / 18.79 | 21.37 / 21.07 | 22.98 / 22.74 |
| almost-funnels, t ≤ 6 | 912 / 1,098 / 1,448 | 5.98 / 5.85 | 6.51 / 6.37 | 6.96 / 6.83 |
| two-level, move structure | 1,068 / 1,183 / 1,362 | 6.03† / 5.89† | 6.43 / 6.30 | 7.15† / 6.98 |
| almost-funnels, g = 4 | 1,087 / 1,206 / 1,391 | 5.51 / 5.58 | 5.82 / 5.78 | 6.34 / 6.30 |
| almost-funnels, g = 16 | 1,075 / 1,192 / 1,375 | 5.58 / 5.55 | 5.86 / 5.82 | 6.40 / 6.28 |
| almost-funnels, g = 64 | 1,071 / 1,187 / 1,367 | 5.90 / 5.74 | 6.22 / 6.01 | 6.79† / 6.60 |

## Table 6, with huge pages (GLIBC_TUNABLES=glibc.malloc.hugetlb=1)

| | MB | 16 | 32 | 64 |
|---|---|---:|---:|---:|
| our move structure | 668 / 716 / 791 | 15.99 / 15.62 | 17.70 / 17.45 | 18.98 / 18.77 |
| almost-funnels, t ≤ 6 | 912 / 1,098 / 1,448 | 5.00 / 4.88 | 5.37 / 5.29 | 5.70 / 5.60 |
| two-level, move structure | 1,068 / 1,183 / 1,362 | 5.53 / 5.37 | 5.87 / 5.70 | 6.48 / 6.31 |
| almost-funnels, g = 4 | 1,087 / 1,206 / 1,391 | 4.99 / 5.01 | 5.25 / 5.23 | 5.72 / 5.69 |
| almost-funnels, g = 16 | 1,075 / 1,192 / 1,375 | 5.02 / 5.03 | 5.33 / 5.23 | 5.78 / 5.68 |
| almost-funnels, g = 64 | 1,071 / 1,187 / 1,367 | 5.34 / 5.19 | 5.62 / 5.44 | 6.19 / 5.95 |

## Table 7, without huge pages

| | MB | 16 | 32 | 64 |
|---|---|---:|---:|---:|
| two-level, move structure | 1,068 / 1,183 / 1,362 | 6.03† / 5.89† | 6.43 / 6.30 | 7.15† / 6.98 |
| θ = 80% | 1,217 / 1,356 / 1,574 | 3.21 / 3.17 | 3.11 / 3.03 | 3.16 / 3.04 |
| θ = 80%, with the first-run table (new) | 1,217 / 1,356 / 1,574 | 3.10 / 3.09 | 3.01 / 2.95 | 3.08 / 2.97 |
| θ = 90% | 1,207 / 1,339 / 1,545 | 3.20 / 3.17 | 3.15 / 3.06 | 3.21 / 3.12 |
| θ = 90%, with the first-run table (new) | 1,207 / 1,339 / 1,545 | 3.12 / 3.10 | 3.03 / 2.98 | 3.10 / 3.03 |
| θ = 95% | 1,199 / 1,330 / 1,531 | 3.24 / 3.19 | 3.20 / 3.12 | 3.29 / 3.21 |
| θ = 95%, with the first-run table (new) | 1,199 / 1,330 / 1,531 | 3.13 / 3.13 | 3.08 / 3.03 | 3.17 / 3.09 |

## Table 7, with huge pages (GLIBC_TUNABLES=glibc.malloc.hugetlb=1)

| | MB | 16 | 32 | 64 |
|---|---|---:|---:|---:|
| two-level, move structure | 1,068 / 1,183 / 1,362 | 5.53 / 5.37 | 5.87 / 5.70 | 6.48 / 6.31 |
| θ = 80% | 1,217 / 1,356 / 1,574 | 2.87 / 2.81 | 2.77 / 2.68 | 2.80 / 2.71 |
| θ = 80%, with the first-run table (new) | 1,217 / 1,356 / 1,574 | 2.75 / 2.73 | 2.65 / 2.62 | 2.69 / 2.63 |
| θ = 90% | 1,207 / 1,339 / 1,545 | 2.86 / 2.82 | 2.80 / 2.73 | 2.86 / 2.77 |
| θ = 90%, with the first-run table (new) | 1,207 / 1,339 / 1,545 | 2.75 / 2.74 | 2.68 / 2.63 | 2.75 / 2.68 |
| θ = 95% | 1,199 / 1,330 / 1,531 | 2.91 / 2.86 | 2.85 / 2.78 | 2.94 / 2.85 |
| θ = 95%, with the first-run table (new) | 1,199 / 1,330 / 1,531 | 2.77 / 2.77 | 2.74 / 2.68 | 2.81 / 2.74 |

## Table 8, without huge pages

| | MB | 16 | 32 | 64 |
|---|---|---:|---:|---:|
| two-level, move structure | 1,068 / 1,183 / 1,362 | 6.03† / 5.89† | 6.43 / 6.30 | 7.15† / 6.98 |
| with encoding (▷) | 1,070 / 1,186 / 1,365 | 3.65 / 3.62 | 3.66 / 3.57 | 3.92 / 3.81 |
| with encoding (▷), with the first-run table (new) | 1,070 / 1,186 / 1,365 | 3.49 / 3.47 | 3.50 / 3.43 | 3.75 / 3.66 |
| with encoding (▷), with the table and lookahead (d = 2) (new) | 1,070 / 1,186 / 1,365 | 3.32 / 3.34 | 3.39 / 3.36 | 3.67 / 3.63 |

## Table 8, with huge pages (GLIBC_TUNABLES=glibc.malloc.hugetlb=1)

| | MB | 16 | 32 | 64 |
|---|---|---:|---:|---:|
| two-level, move structure | 1,068 / 1,183 / 1,362 | 5.53 / 5.37 | 5.87 / 5.70 | 6.48 / 6.31 |
| with encoding (▷) | 1,070 / 1,186 / 1,365 | 3.31 / 3.25 | 3.30 / 3.20 | 3.50 / 3.42 |
| with encoding (▷), with the first-run table (new) | 1,070 / 1,186 / 1,365 | 3.11 / 3.10 | 3.12 / 3.06 | 3.33 / 3.26 |
| with encoding (▷), with the table and lookahead (d = 2) (new) | 1,070 / 1,186 / 1,365 | 2.97 / 2.97 | 3.02 / 2.99 | 3.26 / 3.20 |

## Table 9 (summary, BML), without huge pages

| index, method | 16 | 32 | 64 | MB |
|---|---:|---:|---:|---|
| ropebwt3 | 65.44 | 73.89 | 80.70 | 165 / 187 / 223 |
| Movi 2 | 26.25† | 30.58 | 35.90 | 581 / 615 / 671 |
| STPD-index | 39.07 | 44.08 | — | 264 / 314 / — |
| RLCSA | 39.87† | 48.48 | 53.09 | 579 / 1,193 / 1,301 |
| our move structure | 19.25 | 21.37 | 22.98 | 668 / 716 / 791 |
| almost-funnels, windows of 25 | 13.31 | 14.62 | 15.59 | 801 / 896 / 1,064 |
| almost-funnels, windows of 100 | 7.73 | 8.37 | 8.90 | 723 / 784 / 885 |
| almost-funnels, t ≤ 6 | 5.98 | 6.51 | 6.96 | 912 / 1,098 / 1,448 |
| two-level, move structure | 6.03† | 6.43 | 7.15† | 1,068 / 1,183 / 1,362 |
| two-level, move structure, with the first-run table (new) | 5.47 | 5.74 | 6.36 | 1,068 / 1,183 / 1,362 |
| two-level, move structure, with the table and lookahead (d = 2) (new) | 4.29 | 4.45 | 4.83 | 1,068 / 1,183 / 1,362 |
| two-level, move structure, with lookahead alone (d = 4) (new) | 5.75† | 6.07 | 6.74 | 1,068 / 1,183 / 1,362 |
| STPD of the parse | 4.55 | 4.74 | 5.08 | 2,660 / 4,273 / 6,303 |
| almost-funnels, g = 64 | 5.90 | 6.22 | 6.79† | 1,071 / 1,187 / 1,367 |
| almost-funnels, g = 16 | 5.58 | 5.86 | 6.40 | 1,075 / 1,192 / 1,375 |
| almost-funnels, g = 4 | 5.51 | 5.82 | 6.34 | 1,087 / 1,206 / 1,391 |
| θ = 95% | 3.24 | 3.20 | 3.29 | 1,199 / 1,330 / 1,531 |
| θ = 95%, with the first-run table (new) | 3.13 | 3.08 | 3.17 | 1,199 / 1,330 / 1,531 |
| θ = 90% | 3.20 | 3.15 | 3.21 | 1,207 / 1,339 / 1,545 |
| θ = 90%, with the first-run table (new) | 3.12 | 3.03 | 3.10 | 1,207 / 1,339 / 1,545 |
| θ = 80% | 3.21 | 3.11 | 3.16 | 1,217 / 1,356 / 1,574 |
| θ = 80%, with the first-run table (new) | 3.10 | 3.01 | 3.08 | 1,217 / 1,356 / 1,574 |
| with encoding (▷) | 3.65 | 3.66 | 3.92 | 1,070 / 1,186 / 1,365 |
| with encoding (▷), with the first-run table (new) | 3.49 | 3.50 | 3.75 | 1,070 / 1,186 / 1,365 |
| with encoding (▷), with the table and lookahead (d = 2) (new) | 3.32 | 3.39 | 3.67 | 1,070 / 1,186 / 1,365 |

## Table 9 (summary, BML), with huge pages (GLIBC_TUNABLES=glibc.malloc.hugetlb=1)

| index, method | 16 | 32 | 64 | MB |
|---|---:|---:|---:|---|
| ropebwt3 | 59.94 | 67.72 | 73.80 | 165 / 187 / 223 |
| Movi 2 | 23.26 | 27.13 | 32.07 | 581 / 615 / 671 |
| STPD-index | 38.64 | 43.58 | — | 264 / 314 / — |
| RLCSA | 33.30 | 39.26 | 42.63 | 579 / 1,193 / 1,301 |
| our move structure | 15.99 | 17.70 | 18.98 | 668 / 716 / 791 |
| almost-funnels, windows of 25 | 11.27 | 12.28 | 13.10 | 801 / 896 / 1,064 |
| almost-funnels, windows of 100 | 6.52 | 7.03 | 7.44 | 723 / 784 / 885 |
| almost-funnels, t ≤ 6 | 5.00 | 5.37 | 5.70 | 912 / 1,098 / 1,448 |
| two-level, move structure | 5.53 | 5.87 | 6.48 | 1,068 / 1,183 / 1,362 |
| two-level, move structure, with the first-run table (new) | 4.94 | 5.20 | 5.74 | 1,068 / 1,183 / 1,362 |
| two-level, move structure, with the table and lookahead (d = 2) (new) | 3.91 | 4.08 | 4.39 | 1,068 / 1,183 / 1,362 |
| two-level, move structure, with lookahead alone (d = 4) (new) | 5.33 | 5.54 | 6.17 | 1,068 / 1,183 / 1,362 |
| STPD of the parse | 4.01 | 4.21 | 4.49 | 2,660 / 4,273 / 6,303 |
| almost-funnels, g = 64 | 5.34 | 5.62 | 6.19 | 1,071 / 1,187 / 1,367 |
| almost-funnels, g = 16 | 5.02 | 5.33 | 5.78 | 1,075 / 1,192 / 1,375 |
| almost-funnels, g = 4 | 4.99 | 5.25 | 5.72 | 1,087 / 1,206 / 1,391 |
| θ = 95% | 2.91 | 2.85 | 2.94 | 1,199 / 1,330 / 1,531 |
| θ = 95%, with the first-run table (new) | 2.77 | 2.74 | 2.81 | 1,199 / 1,330 / 1,531 |
| θ = 90% | 2.86 | 2.80 | 2.86 | 1,207 / 1,339 / 1,545 |
| θ = 90%, with the first-run table (new) | 2.75 | 2.68 | 2.75 | 1,207 / 1,339 / 1,545 |
| θ = 80% | 2.87 | 2.77 | 2.80 | 1,217 / 1,356 / 1,574 |
| θ = 80%, with the first-run table (new) | 2.75 | 2.65 | 2.69 | 1,217 / 1,356 / 1,574 |
| with encoding (▷) | 3.31 | 3.30 | 3.50 | 1,070 / 1,186 / 1,365 |
| with encoding (▷), with the first-run table (new) | 3.11 | 3.12 | 3.33 | 1,070 / 1,186 / 1,365 |
| with encoding (▷), with the table and lookahead (d = 2) (new) | 2.97 | 3.02 | 3.26 | 1,070 / 1,186 / 1,365 |

## Headline ratios: plain move structure over the best two-level row

| mode | algorithm | rows | 16 | 32 | 64 |
|---|---|---|---:|---:|---:|
| without huge pages | BML | the paper's rows | 6.01 (θ = 90%) | 6.87 (θ = 80%) | 7.27 (θ = 80%) |
| without huge pages | BML | with the new rows | 6.20 (θ = 80%, with the first-run table (new)) | 7.10 (θ = 80%, with the first-run table (new)) | 7.47 (θ = 80%, with the first-run table (new)) |
| without huge pages | FB | the paper's rows | 5.94 (θ = 90%) | 6.96 (θ = 80%) | 7.47 (θ = 80%) |
| without huge pages | FB | with the new rows | 6.07 (θ = 80%, with the first-run table (new)) | 7.14 (θ = 80%, with the first-run table (new)) | 7.66 (θ = 80%, with the first-run table (new)) |
| with huge pages | BML | the paper's rows | 5.58 (θ = 90%) | 6.40 (θ = 80%) | 6.77 (θ = 80%) |
| with huge pages | BML | with the new rows | 5.82 (θ = 80%, with the first-run table (new)) | 6.67 (θ = 80%, with the first-run table (new)) | 7.06 (θ = 80%, with the first-run table (new)) |
| with huge pages | FB | the paper's rows | 5.56 (θ = 80%) | 6.51 (θ = 80%) | 6.94 (θ = 80%) |
| with huge pages | FB | with the new rows | 5.71 (θ = 80%, with the first-run table (new)) | 6.67 (θ = 80%, with the first-run table (new)) | 7.15 (θ = 80%, with the first-run table (new)) |

## Stability

Spread = (max − min) / min of the first 3 runs of each row, size and algorithm.

| mode | cells | over 3% | over 1% | median spread | largest spread |
|---|---:|---:|---:|---:|---|
| without huge pages | 154 | 10 | 33 | 0.47% | 9.8% (RLCSA, FB, 16) |
| with huge pages | 154 | 7 | 32 | 0.53% | 5.1% (two-level, RLCSA, BML, 32) |

Cells over 3% without huge pages (all runs, s):

- 16, Movi 2, BML: 27.48, 26.24, 26.25, 26.38, 26.21; with huge pages 23.91, 23.26, 23.19
- 16, RLCSA, BML: 43.20, 39.74, 39.87, 39.94, 39.56; with huge pages 34.37, 33.13, 33.30
- 16, RLCSA, FB: 42.05, 38.31, 38.37, 38.41, 38.35; with huge pages 32.50, 32.20, 32.21
- 16, two-level, move structure, BML: 6.23, 6.09, 6.01, 6.03, 6.03; with huge pages 5.51, 5.63, 5.53
- 16, two-level, move structure, FB: 5.89, 5.89, 6.08, 5.87, 6.01; with huge pages 5.37, 5.32, 5.37
- 16, two-level, move structure, with lookahead alone (d = 4) (new), BML: 5.73, 5.92, 5.75, 5.71, 5.79; with huge pages 5.33, 5.30, 5.35
- 16, two-level, move structure, with lookahead alone (d = 4) (new), FB: 5.63, 6.00, 5.64, 5.61, 5.64; with huge pages 5.12, 5.11, 5.12
- 16, STPD of the parse, FB: 5.14, 5.41, 5.09, 5.12, 5.11; with huge pages 4.40, 4.45, 4.55
- 64, two-level, move structure, BML: 7.23, 7.34, 7.12, 7.13, 7.15; with huge pages 6.49, 6.43, 6.48
- 64, almost-funnels, g = 64, BML: 7.00, 6.76, 6.79, 6.86, 6.77; with huge pages 6.23, 6.17, 6.19

## Shares of resident memory in huge pages (with huge pages; range over all runs of the row)

| row | 16 | 32 | 64 |
|---|---:|---:|---:|
| ropebwt3 | 87–96% | 88–97% | 88–97% |
| Movi 2 | 98–98% | 98% | 98% |
| STPD-index | 96% | 96–97% | — |
| RLCSA | 92–94% | 95–97% | 96–97% |
| our move structure | 90% | 86–91% | 92% |
| almost-funnels, windows of 25 | 89% | 90% | 91% |
| almost-funnels, windows of 100 | 89% | 89–90% | 90–90% |
| almost-funnels, t ≤ 6 | 88–89% | 90% | 92–92% |
| two-level, move structure | 82–85% | 82–83% | 84–85% |
| two-level, move structure, with the first-run table (new) | 82–84% | 87% | 84–85% |
| two-level, move structure, with the table and lookahead (d = 2) (new) | 82–83% | 84% | 86% |
| two-level, move structure, with lookahead alone (d = 4) (new) | 82–82% | 84–86% | 84–85% |
| STPD of the parse | 88–93% | 82–95% | 81–87% |
| almost-funnels, g = 64 | 83% | 84–87% | 85% |
| almost-funnels, g = 16 | 82% | 84% | 85–85% |
| almost-funnels, g = 4 | 82–83% | 84–87% | 85% |
| θ = 95% | 86% | 87% | 88–88% |
| θ = 95%, with the first-run table (new) | 87% | 88% | 89% |
| θ = 90% | 86–87% | 87% | 88–89% |
| θ = 90%, with the first-run table (new) | 86–87% | 88% | 89% |
| θ = 80% | 86–87% | 88% | 88–89% |
| θ = 80%, with the first-run table (new) | 86–87% | 88% | 89–90% |
| with encoding (▷) | 84% | 86% | 84–87% |
| with encoding (▷), with the first-run table (new) | 85% | 86–87% | 85% |
| with encoding (▷), with the table and lookahead (d = 2) (new) | 85% | 86% | 85% |

## Sizes of the new rows

| row | MB of files loaded (16 / 32 / 64) | first-run table built at load, MB (16 / 32 / 64) |
|---|---|---|
| two-level, move structure, with the first-run table (new) | 1,068 / 1,183 / 1,362 | 77.9 / 90.4 / 108.7 |
| two-level, move structure, with the table and lookahead (d = 2) (new) | 1,068 / 1,183 / 1,362 | 77.9 / 90.4 / 108.7 |
| two-level, move structure, with lookahead alone (d = 4) (new) | 1,068 / 1,183 / 1,362 | — / — / — |
| θ = 95%, with the first-run table (new) | 1,199 / 1,330 / 1,531 | 77.9 / 90.4 / 108.7 |
| θ = 90%, with the first-run table (new) | 1,207 / 1,339 / 1,545 | 77.9 / 90.4 / 108.7 |
| θ = 80%, with the first-run table (new) | 1,217 / 1,356 / 1,574 | 77.9 / 90.4 / 108.7 |
| with encoding (▷), with the first-run table (new) | 1,070 / 1,186 / 1,365 | 78.8 / 91.0 / 108.9 |
| with encoding (▷), with the table and lookahead (d = 2) (new) | 1,070 / 1,186 / 1,365 | 78.8 / 91.0 / 108.9 |
