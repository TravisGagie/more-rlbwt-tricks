# The October 2026 runs of ropebwt3 and Movi 2 (with the earlier implementation's scripts)

Runs of 2026-10-27/28 with the earlier two-level code (`twolevel/` at the top of this repository), copied as they were;
nothing here was rerun. The scripts in `twolevel_old/` are the ones that were run (`twolevel/run_final.sh` and
`run_movi_final.sh` are later versions of the first and third).

| file | produced by | what it gives | used in the paper for |
|---|---|---|---|
| `twolevel_old/final/summary.txt` | `run_final.sh` | ropebwt3 at 16 individuals (and an earlier run of plain search and Reportable Shortcuts, below) | ropebwt3's row at 16 in Table 1 and the summary table |
| `twolevel_old/final/summary_extras.txt`, `extras_final.log` | `run_extras_final.sh` | ropebwt3 at 32 and 64 | ropebwt3's rows at 32 and 64 in Table 1 and the summary table |
| `twolevel_old/final/summary_movi*.txt`, `movi_final.log`, `movi_n32.log`, `movi_n64.log` | `run_movi_final.sh` | Movi 2 (commit 9bc0acc, regular index of both strands, ftab k = 10): MEMs with L = 100 and 1000, all MEMs (L = 1), index sizes | Movi 2's rows in Table 1 and the summary table |
| `twolevel_old/final/n*/{bml,fb}.{csv,txt}`, `final.log`, and the plain and shortcut lines of `summary.txt` | `run_final.sh` | an earlier run of plain search and Reportable Shortcuts (τ = 6, ℓ = 25) on its RLBWTs of the text and its reverse, BML and FB, L = 100 and 1000, at 16, 32, 64 individuals | not used by the paper (its speedups come from the later reruns on the move structure, `../chr19/reruns_fixed/` and `../chr19/tsc_text/`); kept because `summary.txt` holds ropebwt3's 16-individual numbers |

How these runs differ from the setup of the rest of `results/chr19/`:
- median of 5 rounds (Movi 2's all-MEMs: one round), pinned to CPU 9, no memory cgroup; the same machine;
- loading excluded: for ropebwt3, the time from "loaded the BWT" to "processed" in its log; for Movi 2, its "processing
  the reads" time; sizes in MiB (bytes / 1,048,576);
- ropebwt3's MEMs equal ours except for reverse-strand matches (5 / 8 / 11 at L = 1000). Movi 2's MEM sets are not
  identical to ours: at 64, L = 1000, it reports 106 MEMs we don't, 17 of them reverse-strand (examples in
  `summary_movi_n64.txt`).
