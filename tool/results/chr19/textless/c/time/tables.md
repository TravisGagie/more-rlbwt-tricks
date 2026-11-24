## Seconds, BML / forward–backward, with huge pages (GLIBC_TUNABLES=glibc.malloc.hugetlb=1)

*Commit hashes in this README refer to the private development history of this code; those commits are not in this repository.*

MB as built (16 / 32 / 64; files loaded, plus the first-run table built at load); median of 3 (5 where the driver ran two more).

| | MB | 16 | 32 | 64 |
|---|---|---:|---:|---:|
| our move structure (paper's row, re-timed) | 668 / 716 / 791 | 16.02 / 15.67 | 17.72 / 17.48 | 19.00 / 18.82 |
| two-level (paper's row, re-timed) | 1,068 / 1,183 / 1,362 | 5.53 / 5.36 | 5.90 / 5.71 | 6.52 / 6.37 |
| ▷ (paper's row, re-timed) | 1,070 / 1,186 / 1,365 | 3.31 / 3.26 | 3.29 / 3.21 | 3.52 / 3.42 |
| heavy paths θ = 80% (paper's row, re-timed) | 1,217 / 1,356 / 1,574 | 2.86 / 2.82 | 2.76 / 2.69 | 2.81 / 2.71 |
| heavy paths θ = 80%, with the table (paper's row, re-timed) | 1,295 / 1,446 / 1,683 | 2.74 / 2.73 | 2.66 / 2.61 | 2.71 / 2.63 |
| **without the index of the text:** two-level | 313 / 369 / 470 | 5.00 / 5.54 | 5.49 / 5.92 | 6.20 / 6.66 |
| \quad with the table and lookahead | 381 / 446 / 558 | 3.40 / 3.71 | 3.63 / 3.95 | 4.00 / 4.33 |
| \quad ▷ | 357 / 428 / 553 | 2.93 / 3.05 | 3.02 / 3.17 | 3.28 / 3.45 |
| \quad ▷, with the tables and lookahead | 436 / 519 / 662 | 2.57 / 2.71 | 2.72 / 2.89 | 2.97 / 3.19 |
| \quad ▷, jump at the right end | 357 / 428 / 553 | 2.77 / 2.92 | 2.92 / 3.09 | 3.18 / 3.40 |
| \quad ▷, jump, tables and lookahead | 436 / 519 / 662 | 2.50 / 2.67 | 2.67 / 2.86 | 2.94 / 3.15 |
| \quad ◇ | 361 / 433 / 561 | 3.17 / 3.34 | 3.36 / 3.61 | 3.90 / 4.15 |
| \quad heavy paths θ = 80% | 462 / 542 / 682 | 2.38 / 2.49 | 2.38 / 2.50 | 2.45 / 2.59 |
| \quad heavy paths θ = 80%, with the table | 530 / 618 / 770 | 2.26 / 2.37 | 2.25 / 2.38 | 2.32 / 2.47 |
| \quad heavy paths θ = 90% | 453 / 525 / 653 | 2.39 / 2.50 | 2.41 / 2.54 | 2.51 / 2.65 |
| \quad heavy paths θ = 90%, with the table | 521 / 601 / 742 | 2.26 / 2.38 | 2.29 / 2.42 | 2.38 / 2.52 |
| \quad heavy paths θ = 95% | 445 / 516 / 639 | 2.42 / 2.54 | 2.47 / 2.60 | 2.58 / 2.73 |
| \quad heavy paths θ = 95%, with the table | 513 / 593 / 728 | 2.29 / 2.41 | 2.33 / 2.46 | 2.44 / 2.60 |
| \quad almost-funnels, g = 4 | 332 / 392 / 499 | 4.48 / 4.81 | 4.82 / 5.21 | 5.36 / 5.68 |
| \quad almost-funnels, g = 16 | 320 / 378 / 483 | 4.59 / 4.77 | 4.89 / 5.13 | 5.46 / 5.68 |
| \quad almost-funnels, g = 64 | 316 / 373 / 476 | 4.85 / 5.11 | 5.19 / 5.44 | 5.83 / 6.09 |

**The re-timed rows against the committed medians** (this session / cf143c3, BML / FB):

| | 16 | 32 | 64 |
|---|---:|---:|---:|
| move | 1.001 / 1.003 | 1.001 / 1.002 | 1.001 / 1.002 |
| nojumps.move | 1.001 / 0.999 | 1.005 / 1.002 | 1.007 / 1.009 |
| unique | 1.000 / 1.002 | 0.998 / 1.001 | 1.005 / 1.002 |
| heavy08 | 0.997 / 1.004 | 0.998 / 1.002 | 1.002 / 1.001 |
| heavy08+table | 0.997 / 0.999 | 1.001 / 0.999 | 1.006 / 1.000 |

## Seconds, BML / forward–backward, without huge pages

MB as built (16 / 32 / 64; files loaded, plus the first-run table built at load); median of 3 (5 where the driver ran two more).

| | MB | 16 | 32 | 64 |
|---|---|---:|---:|---:|
| our move structure (paper's row, re-timed) | 668 / 716 / 791 | 19.29 / 18.85 | 21.39 / 21.10 | 22.99 / 22.79 |
| two-level (paper's row, re-timed) | 1,068 / 1,183 / 1,362 | 5.99 / 5.89 | 6.41 / 6.26 | 7.14 / 7.00 |
| ▷ (paper's row, re-timed) | 1,070 / 1,186 / 1,365 | 3.67 / 3.62 | 3.68 / 3.57 | 3.96 / 3.84 |
| heavy paths θ = 80% (paper's row, re-timed) | 1,217 / 1,356 / 1,574 | 3.21 / 3.17 | 3.10 / 3.02 | 3.18 / 3.07 |
| heavy paths θ = 80%, with the table (paper's row, re-timed) | 1,295 / 1,446 / 1,683 | 3.10 / 3.09 | 3.01 / 2.95 | 3.07 / 2.99 |
| **without the index of the text:** two-level | 313 / 369 / 470 | 5.39 / 5.92 | 5.89 / 6.41 | 6.65 / 7.19 |
| \quad with the table and lookahead | 381 / 446 / 558 | 3.66 / 3.99 | 3.91 / 4.25 | 4.34 / 4.67 |
| \quad ▷ | 357 / 428 / 553 | 3.17 / 3.29 | 3.29 / 3.44 | 3.59 / 3.79 |
| \quad ▷, with the tables and lookahead | 436 / 519 / 662 | 2.82 / 2.96 | 2.98 / 3.16 | 3.30 / 3.51 |
| \quad ▷, jump at the right end | 357 / 428 / 553 | 3.01 / 3.16 | 3.18 / 3.36 | 3.51 / 3.72 |
| \quad ▷, jump, tables and lookahead | 436 / 519 / 662 | 2.74 / 2.91 | 2.93 / 3.13 | 3.26 / 3.48 |
| \quad ◇ | 361 / 433 / 561 | 3.45 / 3.63 | 3.68 / 3.94 | 4.29 / 4.58 |
| \quad heavy paths θ = 80% | 462 / 542 / 682 | 2.64 / 2.74 | 2.65 / 2.76 | 2.72 / 2.86 |
| \quad heavy paths θ = 80%, with the table | 530 / 618 / 770 | 2.52 / 2.63 | 2.52 / 2.64 | 2.61 / 2.75 |
| \quad heavy paths θ = 90% | 453 / 525 / 653 | 2.65 / 2.75 | 2.67 / 2.79 | 2.79 / 2.92 |
| \quad heavy paths θ = 90%, with the table | 521 / 601 / 742 | 2.51 / 2.63 | 2.54 / 2.68 | 2.66 / 2.81 |
| \quad heavy paths θ = 95% | 445 / 516 / 639 | 2.67 / 2.78 | 2.71 / 2.86 | 2.87 / 3.01 |
| \quad heavy paths θ = 95%, with the table | 513 / 593 / 728 | 2.54 / 2.66 | 2.58 / 2.73 | 2.72 / 2.88 |
| \quad almost-funnels, g = 4 | 332 / 392 / 499 | 4.90 / 5.22 | 5.30 / 5.62 | 5.92 / 6.24 |
| \quad almost-funnels, g = 16 | 320 / 378 / 483 | 4.95 / 5.22 | 5.33 / 5.59 | 5.95 / 6.21 |
| \quad almost-funnels, g = 64 | 316 / 373 / 476 | 5.28 / 5.54 | 5.70 / 5.97 | 6.37 / 6.65 |

**The re-timed rows against the committed medians** (this session / cf143c3, BML / FB):

| | 16 | 32 | 64 |
|---|---:|---:|---:|
| move | 1.002 / 1.003 | 1.001 / 1.001 | 1.001 / 1.002 |
| nojumps.move | 0.994 / 0.999 | 0.997 / 0.994 | 0.999 / 1.004 |
| unique | 1.004 / 1.001 | 1.005 / 1.001 | 1.010 / 1.008 |
| heavy08 | 1.003 / 0.999 | 0.998 / 0.999 | 1.005 / 1.009 |
| heavy08+table | 1.000 / 0.998 | 1.000 / 0.999 | 0.999 / 1.008 |

## The largest resident size (MB, with huge pages / without)

| | 16 | 32 | 64 |
|---|---:|---:|---:|
| our move structure (paper's row, re-timed) | 867 / 864 | 912 / 910 | 984 / 982 |
| two-level (paper's row, re-timed) | 1,406 / 1,414 | 1,520 / 1,525 | 1,692 / 1,698 |
| ▷ (paper's row, re-timed) | 1,345 / 1,416 | 1,457 / 1,528 | 1,694 / 1,701 |
| heavy paths θ = 80% (paper's row, re-timed) | 1,544 / 1,534 | 1,672 / 1,669 | 1,948 / 1,921 |
| heavy paths θ = 80%, with the table (paper's row, re-timed) | 1,611 / 1,648 | 1,759 / 1,795 | 2,053 / 2,025 |
| **without the index of the text:** two-level | 672 / 677 | 727 / 731 | 824 / 829 |
| \quad with the table and lookahead | 738 / 742 | 799 / 804 | 908 / 913 |
| \quad ▷ | 658 / 647 | 717 / 715 | 839 / 909 |
| \quad ▷, with the tables and lookahead | 734 / 722 | 805 / 801 | 1,006 / 1,013 |
| \quad ▷, jump at the right end | 659 / 647 | 727 / 715 | 839 / 909 |
| \quad ▷, jump, tables and lookahead | 733 / 722 | 805 / 801 | 943 / 1,013 |
| \quad ◇ | 654 / 651 | 724 / 793 | 911 / 917 |
| \quad heavy paths θ = 80% | 855 / 805 | 932 / 875 | 1,024 / 1,011 |
| \quad heavy paths θ = 80%, with the table | 875 / 862 | 960 / 947 | 1,099 / 1,095 |
| \quad heavy paths θ = 90% | 844 / 827 | 915 / 866 | 994 / 982 |
| \quad heavy paths θ = 90%, with the table | 866 / 852 | 942 / 930 | 1,070 / 1,066 |
| \quad heavy paths θ = 95% | 836 / 820 | 863 / 902 | 980 / 968 |
| \quad heavy paths θ = 95%, with the table | 857 / 845 | 933 / 922 | 1,056 / 1,052 |
| \quad almost-funnels, g = 4 | 740 / 756 | 798 / 783 | 902 / 874 |
| \quad almost-funnels, g = 16 | 728 / 785 | 784 / 770 | 886 / 858 |
| \quad almost-funnels, g = 64 | 724 / 791 | 780 / 752 | 880 / 851 |

Largest spread between the runs of one row, size, algorithm and mode: 15.9%.
