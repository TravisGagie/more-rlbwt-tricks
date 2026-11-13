# Shortcuts over phrases on the two-level index, chr19 (2026-11-08, night)

The design is in `src/pshortcut.hpp`; the builder is `src/pshortcut_build.hpp`.

**How a shortcut works.**
- **What it is:** a shortcut is attached to a phrase w and crosses a stretch α of k ≥ 2 phrases that often precedes w.
- **When it's taken:** backward search over the parse holds the parse interval of a pattern suffix Z that starts with the complete phrase w. If the pattern's k phrases before Z are α (checked with a 64-bit fingerprint of the phrase ids), one rank query gives the interval of αZ. That interval starts at αw's first row plus the number of w's rows before Z's that are preceded by α, since those rows are in the same order as αw's.
- **Failing midway:** if the interval is empty, αZ does not occur. The search then goes on with ordinary parse steps.
- **What's stored per shortcut:** k, αw's first row, w's first row, the fingerprint, and w's exception rows (those preceded by α, or the rest, whichever are fewer). The exception rows go in one Elias–Fano list over all parse rows.
- **What isn't used:** with shortcuts, block steps are off. Shortcuts don't support locating.

**Placement.** Both kinds are built from the `.lvl` alone (`katka index --pshortcuts SPEC --pshortcuts-out FILE PREFIX`), so the same index serves both:
- **Heavy paths, `heavy:θ`:**
  - Edges: v → u, weighted by how often u precedes v.
  - The walk: from each phrase w occurring at least twice, follow the heaviest edge (u_1 is w's most frequent predecessor, u_2 is u_1's, and so on) while at least θ of w's occurrences are preceded by u_t … u_1, up to 64 phrases.
  - The shortcut: α = u_k … u_1, kept if k ≥ 2.
  - θ = 0.8, 0.9 and 0.95.
- **From the reference's parse, `ref:T`:**
  - The reference is the first sequence (CHM13, forward strand): the parse up to the first phrase that is not over A/C/G/T.
  - Every T-th phrase w of it gets the stretch of the T phrases before it (the first one, if w recurs).
  - T = 4, 16 and 64.

**Setup:** Table 1's.
- **Search:** `find -L 1000 --format mems --csa --twolevel --stats -j 1 --pshortcuts FILE` on the two query haplotypes.
- **Conditions:** pinned to CPU 0, in an 11 GB cgroup without swap, median of 3 repeats.
- **Correctness:** every run's output, MEMs and BWT intervals, is identical to the paper's, which equals plain BML's. The selftest also checks both placements against plain search on synthetic data.
- **Files:** scripts `experiments/pshortcut_timing.sh` and `pshortcut_table.py`; table `table.md` and `runs.tsv`; logs in `logs/`; build logs in `build_logs/`.
- The shortcut files are in `~/research/rlbwt-tricks/chr19/psc/`.

**Columns of `table.md`:**
- **MB:** the `.lvl` plus the shortcuts.
- **Entrances met:** the times the search stood at a phrase with a shortcut.
- **Not alpha:** of those, the times the pattern's preceding phrases were not α.
- **Taken:** the shortcuts crossed, and the phrases they crossed.
- **Failed midway:** α matched but αZ does not occur.
