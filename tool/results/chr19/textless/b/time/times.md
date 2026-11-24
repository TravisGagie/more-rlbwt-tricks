## Seconds, BML, without huge pages

*Commit hashes in this README refer to the private development history of this code; those commits are not in this repository.*

Median of 3 (5 where final.sh ran two more); in parentheses the committed median; then the largest resident size (MB) during the run.

| | 16 | 32 | 64 |
|---|---:|---:|---:|
| two-level, move structure (the paper's) | 6.05 (6.03), 1,414 MB | 6.47 (6.43), 1,525 MB | 7.17 (7.15), 1,698 MB |
| two-level, **without the index of the text** | 5.49, 836 MB | 5.95, 914 MB | 6.73, 1,041 MB |
| ▷, move structure (the paper's) | 3.67 (3.65), 1,416 MB | 3.67 (3.66), 1,528 MB | 3.95 (3.92), 1,701 MB |
| two-level, table + lookahead d = 2 | 4.31 (4.29), 1,468 MB | 4.46 (4.45), 1,612 MB | 4.84 (4.83), 1,802 MB |
| two-level without the index of the text, table + lookahead d = 2 | 3.73, 911 MB | 3.97, 1,000 MB | 4.38, 1,092 MB |
| ▷, table + lookahead d = 2 | 3.33 (3.32), 1,491 MB | 3.41 (3.39), 1,615 MB | 3.72 (3.67), 1,744 MB |

**Without the index of the text / with it** (this session)

| | 16 | 32 | 64 |
|---|---:|---:|---:|
| plain | 0.907 | 0.919 | 0.938 |
| table + lookahead | 0.865 | 0.890 | 0.905 |

## Seconds, BML, with huge pages (GLIBC_TUNABLES=glibc.malloc.hugetlb=1)

Median of 3 (5 where final.sh ran two more); in parentheses the committed median; then the largest resident size (MB) during the run.

| | 16 | 32 | 64 |
|---|---:|---:|---:|
| two-level, move structure (the paper's) | 5.62 (5.53), 1,406 MB | 5.89 (5.87), 1,520 MB | 6.55 (6.48), 1,693 MB |
| two-level, **without the index of the text** | 5.06, 830 MB | 5.51, 898 MB | 6.26, 1,034 MB |
| ▷, move structure (the paper's) | 3.31 (3.31), 1,345 MB | 3.31 (3.30), 1,457 MB | 3.51 (3.50), 1,630 MB |
| two-level, table + lookahead d = 2 | 4.00 (3.91), 1,482 MB | 4.11 (4.08), 1,606 MB | 4.44 (4.39), 1,796 MB |
| two-level without the index of the text, table + lookahead d = 2 | 3.47, 904 MB | 3.70, 995 MB | 4.08, 1,138 MB |
| ▷, table + lookahead d = 2 | 3.01 (2.97), 1,419 MB | 3.06 (3.02), 1,544 MB | 3.26 (3.26), 1,799 MB |

**Without the index of the text / with it** (this session)

| | 16 | 32 | 64 |
|---|---:|---:|---:|
| plain | 0.900 | 0.937 | 0.956 |
| table + lookahead | 0.868 | 0.901 | 0.919 |

Largest spread between the runs of one row, size and mode: 10.8%. Re-timed rows against the committed medians: 1.004, 1.016, 1.006, 1.003, 1.003, 1.010, 1.005, 1.000, 1.002, 1.004, 1.007, 1.003, 1.006, 1.023, 1.002, 1.008, 1.003, 1.010, 1.004, 1.013, 1.006, 1.011, 1.016, 1.002.
