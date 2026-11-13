# Our STPD of the parse with an RLZ-compressed parse, chr19, 16 individuals (2026-11-12)

*Commit hashes in this README refer to the private development history of this code; those commits are not in this repository.*

2026-11-12. Script `experiments/stpd_parse_rlz.sh 33`
(logs in `logs/`): BML, L = 1000, one occurrence per MEM (`find --csa --stpd-parse --no-intervals`), 3 interleaved
rounds of all four configurations with the same `bin/katka`, pinned to CPU 0, 11 GB cgroups; every run's MEMs equal
the paper's.

- **(a) greedy RLZ:** `bin/katka index --rlzap=0,0,1000000000` under `~/research/rlbwt-tricks/chr19/rlzg/` (links to
  the c33 files; 49 s, peak 3.0 GB). With an unreachable look-ahead threshold the factorization is the greedy longest
  match (among equally long matches, the one keeping the pointer): 22,811,810 factors, 17,510,796 literals, as 3835d67.
  `make_rlz` checks every phrase id and start against the plain parse before writing. In the tool's format it also
  stores the boundaries (the reference's phrase starts and each factor's text start), which the STPD needs.
- **(b) RLZAP:** the tool's `c33.rptxt` (J = 4, D = 4, M = 2): 12,352,342 factors.

## Time (s, BML, median of 3)

| | runs | median | vs plain parse |
|---|---|---:|---:|
| STPD of the parse, plain parse | 6.16, 6.17, 6.09 | **6.16** | |
| STPD of the parse, greedy RLZ | 9.22, 9.31, 9.22 | **9.22** | +50% |
| STPD of the parse, RLZAP | 9.23, 9.24, 9.17 | **9.23** | +50% |
| two-level, ▷ encoding | 5.85, 5.94, 5.86 | **5.86** | −5% |

Neither RLZ is within 10% of the plain parse, so 32 and 64 were not run. The search does the same work in all three
STPD runs (4,235,527 right-end character steps, 13,936,110 phrase comparisons, 157,005 binary searches with
3,806,497 comparisons, 1,114,294 left-end character steps, 1,814,710 predecessor queries); only the parse oracle's
access differs: each id, phrase start and predecessor goes through the factor holding it.

## Loaded size (MB): what the search uses

| | parse (ids) | boundaries | parse sample | rest of the STPD | text level | total | peak (GB) |
|---|---:|---:|---:|---:|---:|---:|---:|
| plain parse | 661.4 | 205.6 | 33.5 | 399.0 | 888.7 | **2,188.2** | 2.96 |
| greedy RLZ | 115.4 | 41.2 | 33.5 | 399.0 | 888.7 | **1,477.8** | 2.27 |
| RLZAP | 107.9 | 28.4 | 33.5 | 399.0 | 888.7 | **1,457.5** | 2.25 |
| two-level ▷ | | | | | | **980.9** | 1.40 |

- **Parse (ids):** plain, the packed ids; RLZ, the reference's ids plus factor starts in the parse, literal counts,
  literals and pointers (greedy 20.1 + 18.6 + 8.4 + 52.5 + 15.8; RLZAP 20.1 + 11.4 + 6.1 + 55.9 + 14.4).
- **Boundaries:** plain, the phrase starts (Elias–Fano); RLZ, the reference's phrase starts (10.5) plus each factor's
  text start (greedy 30.8, RLZAP 18.0).
- **Rest of the STPD:** dictionary strings 151.2 + their offsets 31.9, entry table 29.8 + C 8.6, character-level
  sample 177.4.
- **Text level:** RLCSA 579.3, B 13.4, dictionary hash 105.2, and the RLFM-index `.rlbwt` 191.3, which `find` opens.
  The φ⁻¹ structures (locating only) are not counted.
- **▷:** the RLFM-index 191.3 + RLCSA 579.3 + `unique/c33u.lvl` 210.3, as in Table 1.
- **Against ▷:** with RLZ the STPD of the parse loads about 1.5 times the ▷ encoding's space (1,477.8 / 980.9 = 1.51
  for greedy RLZ, 1,457.5 / 980.9 = 1.49 for RLZAP); with the plain parse, 2.23 times.
