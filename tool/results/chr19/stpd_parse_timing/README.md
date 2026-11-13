# STPD over the parse and over the text, chr19 (2026-11-08 to 2026-11-09)

*Commit hashes in this README refer to the private development history of this code; those commits are not in this repository.*

`experiments/stpd_parse_timing.sh [NS] [REPEATS]` (with `MODES="parse text"`, or `oracle`) in Table 1's setup: the two
query haplotypes, L = 1000, BML, one thread pinned to one core, an 11 GB cgroup without swap, 3 runs each. The MEMs
(name, start, end) equal the paper's in every run.

| mode | options | 16 | 32 | 64 |
|---|---|---:|---:|---:|
| STPD over the parse, plain parse | `--csa --stpd-parse --no-intervals` | 6.09 s, 2.96 GB | 6.34 s, 4.52 GB | 6.44 s, 6.49 GB |
| STPD over the text | `--stpd --no-intervals` | 3.47 s, 4.96 GB | 3.31 s, 8.88 GB | (does not fit in 11 GB) |
| STPD over the text, read through the parse | `--stpd --parse-oracle --no-intervals` | | | 17.41 s, 4.73 GB |

Medians of 3 search times (loading excluded), and the peak memory of the search. `summary.txt` has every time and each
structure's size (the "components" lines); the logs are in `logs/q<N>.stpd-<mode>.<r>.log`.

- **Builds at 64** (`build129/`): `.ptxt` 407 s, `.stpd` 2,513 s (peak 9.4 GB), `.pstpd` 279 s. `c129.rlbwt` is
  unchanged by them (its sha256 before and after).
- These results were committed in eaf0be0 (2026-11-09). The STPD of the parse was rerun at every size with
  the fixed binary of 2026-11-12 in `../reruns_fixed/` (4.85 / 5.05 / 5.25 s).
