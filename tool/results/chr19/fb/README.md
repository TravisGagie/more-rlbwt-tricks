# Forward–backward search on chr19 (2026-11-10)

Forward–backward search against BML, chr19 (2026-11-10). The summary table is `table.md`.
- **Item 1:** `experiments/fb_timing.sh` (two-level: blocks, unitig jumps, heavy-path shortcuts θ = 0.9; 16/32/64), table `table.md` (`experiments/fb_table.py`).
- **Item 2:** `experiments/fb_stpd_index.sh` (Cenzato et al.'s STPD-index, RLZ + r-index, 8a432a4 unmodified; driver `experiments/stpd_index_mems.cpp --fb`; 16/32).
  - Run log: `stpd_index_run.log`. Its first attempt at 16 hit the 3-hour limit because the driver looped; the fix and the rerun are below it.
  - Counts from one untimed BML and FB run per size: `counts/`.
- **Item 3:** `experiments/fb_stpd_ours.sh` (our STPD over the plain text, BML and FB; 16/32).
- **2026-11-11 request:** `experiments/fb_timing.sh` with configs `plain` (plain backward search on the RLCSA) and `nojumps` (two-level, no block steps), 16/32/64; same-day BML reruns `experiments/bml_retime.sh` (run logs `plain_nojumps_run.log`, `plain_bml_run.log`, `nojumps_bml_run.log`). Rows in `table.md`.
- **Setup:** the main table's: one thread pinned to CPU 0, an 11 GB cgroup without swap, median of 3, excluding load.
- **Correctness:** every run's MEMs equal BML's (the scripts stop otherwise).
- **Logs:** `logs/`.
