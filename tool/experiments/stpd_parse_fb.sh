#!/bin/bash
# Two-level search with our STPD of the parse, BML and forward-backward, chr19, in Table 1's setup
# (2026-11-11, "two-level with an STPD of the parse").  From tool/:
#
#   experiments/stpd_parse_fb.sh [NS] [REPEATS]
#
# NS (default "33 65 129": 16, 32, 64 individuals).  Four configurations of `find -L 1000 --format mems --csa
# --stpd-parse --stats -j 1` on ~/research/rlbwt-tricks/chr19/c<N>:
#   bml.noint, fb.noint   with --no-intervals: one occurrence per MEM (the STPD's), no BWT interval;
#   bml.int,   fb.int     without it: each MEM's BWT interval as well, by an ordinary backward search over the MEM.
# REPEATS (default 3) runs each, pinned to CPU $PIN (default 0), in an 11 GB cgroup without swap.  The MEMs (name,
# start, end) must equal the paper's BML output blocks/q<N>s.blocks.tsv; with intervals, the intervals (columns 4-5)
# must too.  Logs in results/chr19/stpd_parse_fb/logs/q<N>.<config>.<r>.log; the files each search opens (strace,
# one untimed run per size) in results/chr19/stpd_parse_fb/opened.q<N>.txt.
set -euo pipefail
cd "$(dirname "$0")/.."
NS=${1:-"33 65 129"}; REP=${2:-3}
D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; Q=$D/blocks/queries.fa; O=results/chr19/stpd_parse_fb; mkdir -p $O/logs
SCR=$(mktemp -d -p "${TMPDIR:-$HOME/research/rlbwt-tricks/tmp}" stpd_parse_fb.XXXX)   # not /tmp: a 7.5 GB tmpfs here
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"; TS="taskset -c ${PIN:-0}"
for n in $NS; do
  for c in bml.noint fb.noint bml.int fb.int; do
    opt=""; [ ${c%.*} = fb ] && opt="--fb"; [ ${c#*.} = noint ] && opt="$opt --no-intervals"
    if [ $c = bml.noint ]; then   # which files the search reads (untimed)
      $CAP strace -f -e trace=openat -o $SCR/strace bin/katka find -L 1000 --format mems --csa --stpd-parse --stats -j 1 $opt $D/c$n $Q $SCR/out.tsv > /dev/null 2>&1
      grep -o "\"$D/c$n\.[a-z]*\"" $SCR/strace | tr -d '"' | sort -u | while read f; do echo "$f $(stat -c %s $f)"; done > $O/opened.q$n.txt
    fi
    for r in $(seq 1 $REP); do
      log=$O/logs/q$n.$c.$r.log
      echo "[$(date '+%F %T')] c$n $c repeat $r" >&2
      /usr/bin/time -f "wall %e s, peak %M KB" $CAP $TS bin/katka find -L 1000 --format mems --csa --stpd-parse --stats -j 1 $opt $D/c$n $Q $SCR/out.tsv > $log 2>&1
      cmp -s <(cut -f1-3 $SCR/out.tsv) <(cut -f1-3 $D/blocks/q${n}s.blocks.tsv) || { echo "c$n $c: MEMs differ from BML's"; exit 1; }
      if [ ${c#*.} = int ]; then
        cmp -s <(cut -f1-5 $SCR/out.tsv) <(cut -f1-5 $D/blocks/q${n}s.blocks.tsv) || { echo "c$n $c: intervals differ from BML's"; exit 1; }
      fi
    done
    [ $c = bml.noint ] && head -n 3 $SCR/out.tsv > $O/sample.q$n.noint.tsv
    [ $c = bml.int ] && head -n 3 $SCR/out.tsv > $O/sample.q$n.int.tsv
  done
done
rm -rf $SCR
