# More RLBWT Tricks

Code and experiments for the paper *More RLBWT Tricks* (Travis Gagie, [arXiv:2609.40299](https://arxiv.org/abs/2609.40299)).
The tag `more-rlbwt-tricks-arxiv-v3` is the code of arXiv v3.

The paper finds maximal exact matches (MEMs) with run-length compressed BWTs (RLBWTs), by Boyer–Moore–Li (BML) or
forward–backward search, and speeds up backward search in several ways that all return the same MEMs and BWT
intervals as plain backward search:

- **two-level indexing** over a parse of the collection at closed syncmers (k = 51, s = 12), with blocks of
  continuations that a search crosses in one step, or the unique-successor (▷) encoding of blocks;
- **almost-funnels over phrases**: shortcuts on the parse, along heavy paths of predecessors or placed from the
  reference's (CHM13's) parse;
- **a move structure** (as in Movi) for the character steps, alone or at the ends of two-level searches;
- **almost-funnels on the text** (Reportable Shortcuts' shortcuts) over one index of both strands, along CHM13 and its
  reverse complement;
- **our STPD of the parse**, a suffix-tree path decomposition over the parse, compared with the published STPD-index.

The experiments are on chromosome 19 of CHM13 and of 16, 32 and 64 individuals of the Human Pangenome Reference
Consortium (release 2), with two held-out haplotypes as queries.

## Layout

- `tool/`: the implementation of the paper, `katka` (one index of both strands; C++17, make). Its `README.md`
  documents every command and option, and maps the paper to the source files.
  - `tool/CHR19.md`: how to download the data, rebuild every index and rerun every experiment.
  - `tool/experiments/chr19_cpm.sh`: one script that rebuilds and reruns every table, checking every search's MEMs
    and intervals against plain backward search; `tool/experiments/chr19_paper.sh` does the same for v2's tables.
  - `tool/results/`: the paper's own runs, with their logs, tables and READMEs (`tool/results/chr19/` for the
    experiments of v3, each directory with a README saying what was run and how).
- `shortcuts/` and `twolevel/`: the earlier implementation, on forward strands only (Reportable Shortcuts' code, and the
  first two-level index). `tool/results/desktop_archive/` has the October runs of ropebwt3 and Movi 2, made with their scripts.

## Building and checking

```
cd tool
make                 # bin/katka (and test programs); needs a C++17 compiler, make, zlib; libsais is included
./selftest.sh        # about 10 minutes; checks every search configuration against plain backward search
```

Then follow `tool/CHR19.md`. Big-BWT is needed for the BWTs of the real collections; the published STPD-index only
for its own rows.

## Licences

GPL v3 or later (`LICENSE`). Third-party code: libsais (`tool/third_party/libsais/`, Apache License 2.0, its
`LICENSE`).
