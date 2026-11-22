# Part 2: chopped and interleaved plain search on the move structure

## Times, without huge pages (s, BML / FB, median of 3; 16 / 32 / 64 individuals)

| row | 16 | 32 | 64 |
|---|---:|---:|---:|
| plain, move structure, unchanged | 19.20† / 18.80† | 21.36 / 21.06 | 23.01 / 22.75 |
| chopped (64 pieces), one by one | 19.39 / 18.97 | 21.58 / 21.26 | 23.18 / 22.94 |
| chopped and interleaved, K = 4 (extra) | 8.85 / 8.63 | 9.79 / 9.62 | 10.56 / 10.44 |
| chopped and interleaved, K = 6 (extra) | 9.16 / 8.91 | 10.09 / 9.91 | 10.84 / 10.76 |
| chopped and interleaved, K = 8 | 9.39† / 9.15 | 10.44 / 10.27 | 11.24 / 11.12 |
| chopped and interleaved, K = 16 | 11.03 / 10.81† | 12.35† / 11.94 | 13.15 / 12.86 |
| chopped and interleaved, K = 32 | 11.95 / 11.42 | 13.21 / 12.88 | 14.18 / 13.96† |
| heavy paths θ = 80% | 3.21 / 3.16 | 3.12 / 3.03 | 3.17 / 3.06 |
| heavy paths θ = 80%, with the first-run table | 3.11 / 3.09 | 3.01 / 2.95 | 3.07 / 2.99 |

## Times, with huge pages (s, BML / FB, median of 3; 16 / 32 / 64 individuals)

| row | 16 | 32 | 64 |
|---|---:|---:|---:|
| plain, move structure, unchanged | 15.97 / 15.63 | 17.69 / 17.45 | 18.96 / 18.80 |
| chopped (64 pieces), one by one | 16.09 / 15.76 | 17.82 / 17.57 | 19.13 / 18.92 |
| chopped and interleaved, K = 4 (extra) | 7.10 / 6.98 | 7.87 / 7.75 | 8.53 / 8.42 |
| chopped and interleaved, K = 6 (extra) | 6.60 / 6.47 | 7.07 / 6.99 | 7.69 / 7.61 |
| chopped and interleaved, K = 8 | 6.31 / 6.25 | 6.99 / 6.94 | 7.58 / 7.54 |
| chopped and interleaved, K = 16 | 7.27 / 7.14 | 7.87 / 7.86 | 8.62 / 8.82 |
| chopped and interleaved, K = 32 | 8.46 / 8.35 | 9.47 / 9.13 | 10.05 / 9.96 |
| heavy paths θ = 80% | 2.88 / 2.82 | 2.77 / 2.69 | 2.83 / 2.72 |
| heavy paths θ = 80%, with the first-run table | 2.76 / 2.73 | 2.67 / 2.62 | 2.75 / 2.64 |

## Speedups, without huge pages: unchanged / row, and chopped alone / row (BML / FB)

| row | 16 | 32 | 64 |
|---|---|---|---|
| chopped (64 pieces), one by one | 0.99 / 0.99 (over chopped 1.00 / 1.00) | 0.99 / 0.99 (over chopped 1.00 / 1.00) | 0.99 / 0.99 (over chopped 1.00 / 1.00) |
| chopped and interleaved, K = 4 (extra) | 2.17 / 2.18 (over chopped 2.19 / 2.20) | 2.18 / 2.19 (over chopped 2.21 / 2.21) | 2.18 / 2.18 (over chopped 2.19 / 2.20) |
| chopped and interleaved, K = 6 (extra) | 2.10 / 2.11 (over chopped 2.12 / 2.13) | 2.12 / 2.12 (over chopped 2.14 / 2.15) | 2.12 / 2.11 (over chopped 2.14 / 2.13) |
| chopped and interleaved, K = 8 | 2.04 / 2.06 (over chopped 2.07 / 2.07) | 2.05 / 2.05 (over chopped 2.07 / 2.07) | 2.05 / 2.05 (over chopped 2.06 / 2.06) |
| chopped and interleaved, K = 16 | 1.74 / 1.74 (over chopped 1.76 / 1.76) | 1.73 / 1.76 (over chopped 1.75 / 1.78) | 1.75 / 1.77 (over chopped 1.76 / 1.78) |
| chopped and interleaved, K = 32 | 1.61 / 1.65 (over chopped 1.62 / 1.66) | 1.62 / 1.63 (over chopped 1.63 / 1.65) | 1.62 / 1.63 (over chopped 1.64 / 1.64) |

## Speedups, with huge pages: unchanged / row, and chopped alone / row (BML / FB)

| row | 16 | 32 | 64 |
|---|---|---|---|
| chopped (64 pieces), one by one | 0.99 / 0.99 (over chopped 1.00 / 1.00) | 0.99 / 0.99 (over chopped 1.00 / 1.00) | 0.99 / 0.99 (over chopped 1.00 / 1.00) |
| chopped and interleaved, K = 4 (extra) | 2.25 / 2.24 (over chopped 2.27 / 2.26) | 2.25 / 2.25 (over chopped 2.26 / 2.27) | 2.22 / 2.23 (over chopped 2.24 / 2.25) |
| chopped and interleaved, K = 6 (extra) | 2.42 / 2.42 (over chopped 2.44 / 2.43) | 2.50 / 2.50 (over chopped 2.52 / 2.51) | 2.47 / 2.47 (over chopped 2.49 / 2.49) |
| chopped and interleaved, K = 8 | 2.53 / 2.50 (over chopped 2.55 / 2.52) | 2.53 / 2.52 (over chopped 2.55 / 2.53) | 2.50 / 2.49 (over chopped 2.52 / 2.51) |
| chopped and interleaved, K = 16 | 2.20 / 2.19 (over chopped 2.21 / 2.21) | 2.25 / 2.22 (over chopped 2.27 / 2.23) | 2.20 / 2.13 (over chopped 2.22 / 2.14) |
| chopped and interleaved, K = 32 | 1.89 / 1.87 (over chopped 1.90 / 1.89) | 1.87 / 1.91 (over chopped 1.88 / 1.92) | 1.89 / 1.89 (over chopped 1.90 / 1.90) |

## Headline ratios: plain search over the best two-level row of this session (heavy paths θ = 80% with the table)

| mode | algorithm | plain search | 16 | 32 | 64 |
|---|---|---|---:|---:|---:|
| without huge pages | BML | unchanged | 6.17 | 7.10 | 7.49 |
| without huge pages | BML | best of K = 8, 16, 32 | 3.02 (K = 8) | 3.47 (K = 8) | 3.66 (K = 8) |
| without huge pages | BML | best K, with the extra K | 2.84 (K = 4) | 3.25 (K = 4) | 3.44 (K = 4) |
| without huge pages | FB | unchanged | 6.08 | 7.13 | 7.62 |
| without huge pages | FB | best of K = 8, 16, 32 | 2.96 (K = 8) | 3.48 (K = 8) | 3.72 (K = 8) |
| without huge pages | FB | best K, with the extra K | 2.79 (K = 4) | 3.26 (K = 4) | 3.50 (K = 4) |
| with huge pages | BML | unchanged | 5.79 | 6.62 | 6.90 |
| with huge pages | BML | best of K = 8, 16, 32 | 2.29 (K = 8) | 2.62 (K = 8) | 2.76 (K = 8) |
| with huge pages | BML | best K, with the extra K | 2.29 (K = 8) | 2.62 (K = 8) | 2.76 (K = 8) |
| with huge pages | FB | unchanged | 5.72 | 6.67 | 7.11 |
| with huge pages | FB | best of K = 8, 16, 32 | 2.29 (K = 8) | 2.65 (K = 8) | 2.85 (K = 8) |
| with huge pages | FB | best K, with the extra K | 2.29 (K = 8) | 2.65 (K = 8) | 2.85 (K = 8) |

## Every run (s)

| row | individuals | algorithm | mode | runs |
|---|---:|---|---|---|
| plain, move structure, unchanged | 16 | BML | without huge pages | 20.38, 19.24, 19.20, 19.20, 19.20 (spread 6.1%) |
| plain, move structure, unchanged | 16 | BML | with huge pages | 16.92, 15.97, 15.97 (spread 5.9%) |
| plain, move structure, unchanged | 16 | FB | without huge pages | 19.86, 18.80, 18.80, 18.81, 18.77 (spread 5.6%) |
| plain, move structure, unchanged | 16 | FB | with huge pages | 15.73, 15.62, 15.63 (spread 0.7%) |
| plain, move structure, unchanged | 32 | BML | without huge pages | 21.37, 21.35, 21.36 (spread 0.1%) |
| plain, move structure, unchanged | 32 | BML | with huge pages | 17.69, 17.67, 17.69 (spread 0.1%) |
| plain, move structure, unchanged | 32 | FB | without huge pages | 21.06, 21.06, 21.06 (spread 0.0%) |
| plain, move structure, unchanged | 32 | FB | with huge pages | 17.45, 17.45, 17.45 (spread 0.0%) |
| plain, move structure, unchanged | 64 | BML | without huge pages | 23.01, 23.01, 23.17 (spread 0.7%) |
| plain, move structure, unchanged | 64 | BML | with huge pages | 18.97, 18.96, 18.96 (spread 0.0%) |
| plain, move structure, unchanged | 64 | FB | without huge pages | 22.74, 22.75, 22.77 (spread 0.1%) |
| plain, move structure, unchanged | 64 | FB | with huge pages | 18.80, 18.77, 18.80 (spread 0.1%) |
| chopped (64 pieces), one by one | 16 | BML | without huge pages | 19.46, 19.39, 19.39 (spread 0.4%) |
| chopped (64 pieces), one by one | 16 | BML | with huge pages | 16.16, 16.08, 16.09 (spread 0.5%) |
| chopped (64 pieces), one by one | 16 | FB | without huge pages | 19.07, 18.97, 18.97 (spread 0.5%) |
| chopped (64 pieces), one by one | 16 | FB | with huge pages | 15.79, 15.75, 15.76 (spread 0.3%) |
| chopped (64 pieces), one by one | 32 | BML | without huge pages | 21.61, 21.57, 21.58 (spread 0.2%) |
| chopped (64 pieces), one by one | 32 | BML | with huge pages | 17.82, 17.81, 17.82 (spread 0.1%) |
| chopped (64 pieces), one by one | 32 | FB | without huge pages | 21.29, 21.26, 21.23 (spread 0.2%) |
| chopped (64 pieces), one by one | 32 | FB | with huge pages | 17.57, 17.57, 17.58 (spread 0.1%) |
| chopped (64 pieces), one by one | 64 | BML | without huge pages | 23.28, 23.18, 23.17 (spread 0.5%) |
| chopped (64 pieces), one by one | 64 | BML | with huge pages | 19.12, 19.13, 19.13 (spread 0.1%) |
| chopped (64 pieces), one by one | 64 | FB | without huge pages | 22.97, 22.94, 22.93 (spread 0.2%) |
| chopped (64 pieces), one by one | 64 | FB | with huge pages | 18.94, 18.92, 18.92 (spread 0.1%) |
| chopped and interleaved, K = 4 (extra) | 16 | BML | without huge pages | 8.87, 8.84, 8.85 (spread 0.3%) |
| chopped and interleaved, K = 4 (extra) | 16 | BML | with huge pages | 7.17, 7.10, 7.10 (spread 1.0%) |
| chopped and interleaved, K = 4 (extra) | 16 | FB | without huge pages | 8.63, 8.72, 8.62 (spread 1.2%) |
| chopped and interleaved, K = 4 (extra) | 16 | FB | with huge pages | 6.98, 6.95, 7.11 (spread 2.2%) |
| chopped and interleaved, K = 4 (extra) | 32 | BML | without huge pages | 9.79, 9.79, 9.78 (spread 0.1%) |
| chopped and interleaved, K = 4 (extra) | 32 | BML | with huge pages | 7.87, 7.86, 7.99 (spread 1.7%) |
| chopped and interleaved, K = 4 (extra) | 32 | FB | without huge pages | 9.63, 9.62, 9.61 (spread 0.2%) |
| chopped and interleaved, K = 4 (extra) | 32 | FB | with huge pages | 7.75, 7.74, 7.75 (spread 0.1%) |
| chopped and interleaved, K = 4 (extra) | 64 | BML | without huge pages | 10.68, 10.56, 10.56 (spread 1.1%) |
| chopped and interleaved, K = 4 (extra) | 64 | BML | with huge pages | 8.53, 8.50, 8.53 (spread 0.4%) |
| chopped and interleaved, K = 4 (extra) | 64 | FB | without huge pages | 10.44, 10.43, 10.45 (spread 0.2%) |
| chopped and interleaved, K = 4 (extra) | 64 | FB | with huge pages | 8.42, 8.40, 8.42 (spread 0.3%) |
| chopped and interleaved, K = 6 (extra) | 16 | BML | without huge pages | 9.18, 9.16, 9.15 (spread 0.4%) |
| chopped and interleaved, K = 6 (extra) | 16 | BML | with huge pages | 6.63, 6.42, 6.60 (spread 3.4%) |
| chopped and interleaved, K = 6 (extra) | 16 | FB | without huge pages | 8.95, 8.91, 8.88 (spread 0.8%) |
| chopped and interleaved, K = 6 (extra) | 16 | FB | with huge pages | 6.32, 6.47, 6.48 (spread 2.6%) |
| chopped and interleaved, K = 6 (extra) | 32 | BML | without huge pages | 10.18, 10.05, 10.09 (spread 1.2%) |
| chopped and interleaved, K = 6 (extra) | 32 | BML | with huge pages | 7.08, 7.07, 7.07 (spread 0.2%) |
| chopped and interleaved, K = 6 (extra) | 32 | FB | without huge pages | 9.87, 9.92, 9.91 (spread 0.6%) |
| chopped and interleaved, K = 6 (extra) | 32 | FB | with huge pages | 6.99, 6.99, 7.02 (spread 0.5%) |
| chopped and interleaved, K = 6 (extra) | 64 | BML | without huge pages | 10.87, 10.83, 10.84 (spread 0.4%) |
| chopped and interleaved, K = 6 (extra) | 64 | BML | with huge pages | 7.69, 7.67, 7.69 (spread 0.2%) |
| chopped and interleaved, K = 6 (extra) | 64 | FB | without huge pages | 10.78, 10.76, 10.73 (spread 0.4%) |
| chopped and interleaved, K = 6 (extra) | 64 | FB | with huge pages | 7.60, 7.61, 7.61 (spread 0.1%) |
| chopped and interleaved, K = 8 | 16 | BML | without huge pages | 8.68, 9.41, 9.39, 9.41, 9.37 (spread 8.5%) |
| chopped and interleaved, K = 8 | 16 | BML | with huge pages | 6.34, 6.29, 6.31 (spread 0.8%) |
| chopped and interleaved, K = 8 | 16 | FB | without huge pages | 9.23, 9.13, 9.15 (spread 1.2%) |
| chopped and interleaved, K = 8 | 16 | FB | with huge pages | 6.25, 6.21, 6.26 (spread 0.8%) |
| chopped and interleaved, K = 8 | 32 | BML | without huge pages | 10.39, 10.44, 10.44 (spread 0.4%) |
| chopped and interleaved, K = 8 | 32 | BML | with huge pages | 7.05, 6.99, 6.99 (spread 0.8%) |
| chopped and interleaved, K = 8 | 32 | FB | without huge pages | 10.36, 10.22, 10.27 (spread 1.3%) |
| chopped and interleaved, K = 8 | 32 | FB | with huge pages | 6.93, 7.22, 6.94 (spread 4.2%) |
| chopped and interleaved, K = 8 | 64 | BML | without huge pages | 11.23, 11.24, 11.27 (spread 0.4%) |
| chopped and interleaved, K = 8 | 64 | BML | with huge pages | 7.58, 7.57, 7.76 (spread 2.6%) |
| chopped and interleaved, K = 8 | 64 | FB | without huge pages | 11.11, 11.13, 11.12 (spread 0.2%) |
| chopped and interleaved, K = 8 | 64 | FB | with huge pages | 7.54, 7.54, 7.53 (spread 0.1%) |
| chopped and interleaved, K = 16 | 16 | BML | without huge pages | 11.05, 11.03, 10.97 (spread 0.8%) |
| chopped and interleaved, K = 16 | 16 | BML | with huge pages | 7.15, 7.27, 7.32 (spread 2.4%) |
| chopped and interleaved, K = 16 | 16 | FB | without huge pages | 10.23, 10.66, 10.81, 10.81, 10.84 (spread 5.6%) |
| chopped and interleaved, K = 16 | 16 | FB | with huge pages | 6.99, 7.14, 7.15 (spread 2.4%) |
| chopped and interleaved, K = 16 | 32 | BML | without huge pages | 12.81, 12.14, 12.38, 12.35, 12.20 (spread 5.6%) |
| chopped and interleaved, K = 16 | 32 | BML | with huge pages | 7.87, 8.18, 7.82 (spread 4.6%) |
| chopped and interleaved, K = 16 | 32 | FB | without huge pages | 12.18, 11.88, 11.94 (spread 2.5%) |
| chopped and interleaved, K = 16 | 32 | FB | with huge pages | 7.86, 7.95, 7.73 (spread 2.9%) |
| chopped and interleaved, K = 16 | 64 | BML | without huge pages | 13.17, 13.15, 13.03 (spread 1.1%) |
| chopped and interleaved, K = 16 | 64 | BML | with huge pages | 8.62, 8.84, 8.61 (spread 2.6%) |
| chopped and interleaved, K = 16 | 64 | FB | without huge pages | 12.86, 12.95, 12.84 (spread 0.9%) |
| chopped and interleaved, K = 16 | 64 | FB | with huge pages | 8.82, 8.90, 8.76 (spread 1.6%) |
| chopped and interleaved, K = 32 | 16 | BML | without huge pages | 11.95, 11.83, 11.95 (spread 1.0%) |
| chopped and interleaved, K = 32 | 16 | BML | with huge pages | 8.29, 8.52, 8.46 (spread 2.8%) |
| chopped and interleaved, K = 32 | 16 | FB | without huge pages | 11.17, 11.50, 11.42 (spread 3.0%) |
| chopped and interleaved, K = 32 | 16 | FB | with huge pages | 8.54, 8.35, 8.25 (spread 3.6%) |
| chopped and interleaved, K = 32 | 32 | BML | without huge pages | 13.08, 13.46, 13.21 (spread 2.9%) |
| chopped and interleaved, K = 32 | 32 | BML | with huge pages | 9.53, 9.47, 9.28 (spread 2.6%) |
| chopped and interleaved, K = 32 | 32 | FB | without huge pages | 12.88, 12.87, 12.96 (spread 0.7%) |
| chopped and interleaved, K = 32 | 32 | FB | with huge pages | 9.13, 9.28, 9.03 (spread 2.7%) |
| chopped and interleaved, K = 32 | 64 | BML | without huge pages | 14.18, 14.19, 14.16 (spread 0.2%) |
| chopped and interleaved, K = 32 | 64 | BML | with huge pages | 9.97, 10.05, 10.22 (spread 2.5%) |
| chopped and interleaved, K = 32 | 64 | FB | without huge pages | 13.88, 14.31, 14.08, 13.96, 13.93 (spread 3.1%) |
| chopped and interleaved, K = 32 | 64 | FB | with huge pages | 9.84, 9.96, 10.05 (spread 2.2%) |
| heavy paths θ = 80% | 16 | BML | without huge pages | 3.21, 3.21, 3.21 (spread 0.2%) |
| heavy paths θ = 80% | 16 | BML | with huge pages | 2.90, 2.88, 2.86 (spread 1.4%) |
| heavy paths θ = 80% | 16 | FB | without huge pages | 3.16, 3.16, 3.15 (spread 0.3%) |
| heavy paths θ = 80% | 16 | FB | with huge pages | 2.82, 2.81, 2.82 (spread 0.3%) |
| heavy paths θ = 80% | 32 | BML | without huge pages | 3.11, 3.12, 3.12 (spread 0.4%) |
| heavy paths θ = 80% | 32 | BML | with huge pages | 2.80, 2.77, 2.77 (spread 0.9%) |
| heavy paths θ = 80% | 32 | FB | without huge pages | 3.03, 3.03, 3.05 (spread 0.7%) |
| heavy paths θ = 80% | 32 | FB | with huge pages | 2.71, 2.69, 2.69 (spread 0.9%) |
| heavy paths θ = 80% | 64 | BML | without huge pages | 3.17, 3.17, 3.15 (spread 0.4%) |
| heavy paths θ = 80% | 64 | BML | with huge pages | 2.84, 2.83, 2.81 (spread 1.2%) |
| heavy paths θ = 80% | 64 | FB | without huge pages | 3.06, 3.08, 3.06 (spread 0.6%) |
| heavy paths θ = 80% | 64 | FB | with huge pages | 2.72, 2.70, 2.72 (spread 0.5%) |
| heavy paths θ = 80%, with the first-run table | 16 | BML | without huge pages | 3.11, 3.11, 3.11 (spread 0.1%) |
| heavy paths θ = 80%, with the first-run table | 16 | BML | with huge pages | 2.77, 2.75, 2.76 (spread 0.5%) |
| heavy paths θ = 80%, with the first-run table | 16 | FB | without huge pages | 3.09, 3.08, 3.10 (spread 0.4%) |
| heavy paths θ = 80%, with the first-run table | 16 | FB | with huge pages | 2.72, 2.74, 2.73 (spread 0.7%) |
| heavy paths θ = 80%, with the first-run table | 32 | BML | without huge pages | 3.03, 3.01, 3.01 (spread 0.6%) |
| heavy paths θ = 80%, with the first-run table | 32 | BML | with huge pages | 2.67, 2.67, 2.67 (spread 0.2%) |
| heavy paths θ = 80%, with the first-run table | 32 | FB | without huge pages | 2.95, 2.94, 2.95 (spread 0.3%) |
| heavy paths θ = 80%, with the first-run table | 32 | FB | with huge pages | 2.62, 2.62, 2.60 (spread 0.7%) |
| heavy paths θ = 80%, with the first-run table | 64 | BML | without huge pages | 3.07, 3.09, 3.07 (spread 0.8%) |
| heavy paths θ = 80%, with the first-run table | 64 | BML | with huge pages | 2.77, 2.70, 2.75 (spread 2.5%) |
| heavy paths θ = 80%, with the first-run table | 64 | FB | without huge pages | 3.00, 2.98, 2.99 (spread 0.9%) |
| heavy paths θ = 80%, with the first-run table | 64 | FB | with huge pages | 2.64, 2.62, 2.66 (spread 1.5%) |

## Shares of resident memory in huge pages (with huge pages; range over the runs)

| row | 16 | 32 | 64 |
|---|---:|---:|---:|
| plain, move structure, unchanged | 90–90% | 91–91% | 92–92% |
| chopped (64 pieces), one by one | 87–88% | 88–88% | 89–89% |
| chopped and interleaved, K = 4 (extra) | 87–87% | 88–88% | 89–89% |
| chopped and interleaved, K = 6 (extra) | 87–88% | 88–88% | 88–89% |
| chopped and interleaved, K = 8 | 87–87% | 88–88% | 88–88% |
| chopped and interleaved, K = 16 | 86–86% | 87–87% | 88–88% |
| chopped and interleaved, K = 32 | 85–85% | 85–86% | 86–87% |
| heavy paths θ = 80% | 84–87% | 86–88% | 87–89% |
| heavy paths θ = 80%, with the first-run table | 87–87% | 88–88% | 88–89% |
