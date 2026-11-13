#!/bin/bash
# The published STPD-index (github.com/regindex/STPD-index at 8a432a4, its current HEAD; Becker et al., "Compressing
# Suffix Trees by Path Decompositions", arXiv 2506.14734v6) on chr19 in More RLBWT Tricks' Table 1 setup
# (2026-11-09).  From tool/:
#
#   experiments/stpd_index_rerun.sh N [CONFIGS] [REPEATS]      N = 33, 65 or 129 (16, 32, 64 individuals)
#
# CONFIGS (default all four that stpd-build offers): ORACLE:PHI with ORACLE = RLZ (their default and their paper's
# choice: RLZ against the first sequence, both strands, -l st.reflen) or bitpacked (the plain text, 2 bits per
# base), PHI = r-index (their default) or move (move-r's phi; their paper's "st-colex (move-phi)").  Tabulation
# length: their default (-b 15).  phi is only used to locate occurrences: MEM finding doesn't call it, but it is
# part of the index that is loaded.
#
# Build (once per N, in ~/research/rlbwt-tricks/chr19/stpd<N>/), as experiments/stpd_index_chr19.sh, but:
#   - for 32 and 64 the FASTAs are written back from katka's text c<N>.S (experiments/fasta_from_S.py; at 16 this
#     gives a byte-identical st.T to the original FASTAs');
#   - katka's STPD sample of S is built reading the text through the parse (index --parse-oracle, then
#     index --stpd --parse-oracle), which needs much less memory than index --stpd;
#   - st.T.pa and st.T.lcs (5 bytes per character each) go to the external drive through symlinks ($BIG);
#   - stpd-build runs with -k for each configuration, so its inputs are kept for the next one.
# Their stpd-build is unchanged; it holds the whole text in memory.
# Runs: their BML, experiments/stpd_index_mems.cpp (BML on their longest-prefix search, Algorithm 1 of their paper,
# with each candidate sample checked by their O.LCS), for MEMs of length >= 1000 on the two
# query haplotypes, REPEATS (default 3) times, pinned to CPU $PIN (default 0), in an 11 GB cgroup without swap.
# Without --locate or --pieces the driver loads the index with their locate_primary_only flag: the text oracle and
# the binary-search structure, not phi; so the time and peak are those of MEM finding alone.
# Their MEMs are compared with plain BML's (blocks/q<N>s.blocks.tsv, the paper's, equal to plain BML's).
# Logs and summary in results/chr19/stpd_index_rerun/.
set -euo pipefail
cd "$(dirname "$0")/.."
n=$1; CONFIGS=${2:-"RLZ:r-index RLZ:move bitpacked:r-index bitpacked:move"}; REP=${3:-3}
HERE=$PWD; K=$HERE/bin/katka; DRV=$HERE/bin/stpd-index-mems
STPD=${STPD:-$HOME/research/rlbwt-tricks/STPD-index}; BIGBWT=${BIGBWT:-$HOME/research/deps/Big-BWT/bigbwt}
D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; W=$D/stpd$n; BIG=${BIG:-$W/big}   # BIG: room for st.T.pa and st.T.lcs (10 bytes per character)
O=${O:-$HERE/results/chr19/stpd_index_rerun}; mkdir -p $O/logs
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"; TS="taskset -c ${PIN:-0}"
B=$O/build$n.log
st() { echo "[$(date '+%F %T')] $*" >&2; }
# Every step runs in its own cgroup capped at 11 GB without swap, so a step that needs more is killed alone (exit status
# 137 in the log) instead of setting off the system's OOM killer, which on 2026-11-09 took the whole terminal with it.
tm() { local log=$1; shift; echo "== $*" >> $log; local rc=0; /usr/bin/time -a -o $log -f "wall %e s, peak %M KB: $(basename $1) ${2:-} ${3:-}" $CAP "$@" >> $log 2>&1 || rc=$?; tail -1 $log >&2; return $rc; }
tag() { echo "$1" | tr ':' '-'; }
mkdir -p $W; cd $W
need=0; for c in $CONFIGS; do [ -s st.$(tag $c).stpdi ] || need=1; done
if [ $need = 1 ]; then
  if [ ! -s st.T.colex_m ] || [ ! -s st.T.lcs ]; then
    if [ ! -s st.S ] && [ ! -s st.bwt ]; then
      st "c$n: FASTAs, stpd-prep"
      if [ $n = 33 ] && [ -s $D/list33abs.tsv ]; then LIST=$D/list33abs.tsv; else tm $B python3 $HERE/experiments/fasta_from_S.py $D/c$n fa list.tsv; LIST=list.tsv; fi
      tm $B $K stpd-prep $LIST st
      rm -rf fa
    fi
    if [ ! -s st.bwt ]; then st "c$n: Big-BWT"; tm $B $BIGBWT st.S; mv st.S.bwt st.bwt; rm -f st.S.parse* st.S.dict st.S.occ st.S.last st.S.sai st.S.ilist st.S.bwlast st.S.bwsai; fi
    [ -s st.ptxt ] || { st "c$n: index --parse-oracle"; tm $B $K index --parse-oracle st; }
    [ -s st.stpd ] || { st "c$n: index --stpd --parse-oracle"; tm $B $K index --stpd --parse-oracle st; }
    mkdir -p "$BIG"; ln -sf "$BIG/st.T.pa" st.T.pa; ln -sf "$BIG/st.T.lcs" st.T.lcs
    st "c$n: stpd-export"; tm $B $K stpd-export st
  fi
  # -l is the RLZ reference length (the first genome, both strands). For the bitpacked oracle it means something else:
  # stpd-build keeps only that prefix of the text (bitpacked_text_oracle.hpp:59), so it's passed only for RLZ.
  for c in $CONFIGS; do
    t=$(tag $c); [ -s st.$t.stpdi ] && continue
    st "c$n: stpd-build $c"
    tm $B $STPD/build/CLI/stpd-build -i st.T -o st.$t.stpdi -t ${c%%:*} -p ${c##*:} $([ ${c%%:*} = RLZ ] && echo -l $(cat st.reflen)) -k ||
      { st "c$n: stpd-build $c failed (exit $?; 137 = over the 11 GB cap)"; rm -f st.$t.stpdi st.$t.stpdi.param; echo "c$n $t: build did not fit in 11 GB (see build$n.log)" >> $O/summary.txt; continue; }
    grep -q "Suffix Tree path decomposition found" $B || { echo "stpd-build did not use katka's files"; exit 1; }
  done
fi
for c in $CONFIGS; do
  t=$(tag $c); [ -s st.$t.stpdi ] || continue
  for r in $(seq 1 $REP); do
    st "c$n: their BML, $c, repeat $r"
    log=$O/logs/q$n.$t.$r.log
    /usr/bin/time -f "wall %e s, peak %M KB" $CAP $TS $DRV st.$t.stpdi $D/blocks/queries.fa 1000 > $O/q$n.$t.tsv 2> $log
  done
  python3 - $n $O $D $W $REP $t <<'PY' | tee -a $O/summary.txt
import re, statistics, sys, os
n, O, D, W, rep, t = sys.argv[1:7]
ts = []
for r in range(1, int(rep) + 1):
    x = open('%s/logs/q%s.%s.%d.log' % (O, n, t, r)).read()
    ts.append(float(re.search(r'finding them ([\d.]+) s', x).group(1))); last = x.strip().splitlines()[-1]
key = lambda f: {tuple(l.rstrip('\n').split('\t')[:3]) for l in open(f)}
theirs = key('%s/q%s.%s.tsv' % (O, n, t)); ours = key('%s/blocks/q%ss.blocks.tsv' % (D, n))
mb = sum(os.path.getsize('%s/st.%s.stpdi%s' % (W, t, e)) for e in ('', '.param')) / 1e6
b = [s for s in open('%s/build%s.log' % (O, n)).read().split('== ') if '-o st.%s.stpdi' % t in s.split('\n')[0]][-1]
comp = [int(re.search(r'%s size = (\d+) bytes' % k, b).group(1)) / 1e6 for k in ('Random-access data structure', 'Binary search data structure', 'Phi-function data structure')]
bw = re.findall(r'wall ([\d.]+) s, peak (\d+) KB', b)[-1]
tab = re.search(r'Tabulation length = (\d+)', b).group(1)
print('c%s (%d individuals) %s: index file %.1f MB (text oracle %.1f, binary search %.1f [tabulation length %s], phi %.1f; MEM finding loads the first two, %.1f MB); '
      'build %.0f s, peak %.2f GB; their BML %s s (median %.2f); last run %s; MEMs theirs %d, plain BML %d, in both %d'
      % (n, {'33': 16, '65': 32, '129': 64}[n], t, mb, comp[0], comp[1], tab, comp[2], comp[0] + comp[1], float(bw[0]), int(bw[1]) / 1e6,
         ' '.join('%.2f' % x for x in ts), statistics.median(ts), last, len(theirs), len(ours), len(theirs & ours)))
PY
done
