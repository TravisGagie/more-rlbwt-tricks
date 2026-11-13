#!/bin/bash
# Same-day reruns with the fixes of 2026-11-12 ("answer on the STPD profile", A-C) in one binary.  From tool/:
#   experiments/twolevel_ac.sh [NS] [CONFIGS] [REPEATS]
# NS (default "33 65 129"), REPEATS (default 3).  CONFIGS (default all), each BML and FB, `find -L 1000 --format mems
# --stats -j 1` on the two query haplotypes with bin/katka:
#   rlcsa              plain backward search on the RLCSA (--csa);
#   nojumps.{csa,move} two-level, no jumps (--twolevel --no-jumps);
#   unique.{csa,move}  two-level with the unique-successor (▷) encoding (unique/c<N>u);
#   heavy08.{csa,move} two-level with heavy-path almost-funnels, theta = 0.8 (--pshortcuts psc/c<N>.heavy08.psc);
#                      likewise heavy09, heavy095, and ref4, ref16, ref64 (almost-funnels from CHM13's parse, g = 4, 16, 64);
#   rlcsa.move         plain backward search on the move structure alone (--move: loads only c<N>.mv);
#   tsc25t0, tsc100t0, tsc25t6  plain backward search on the move structure with almost-funnels on the text along CHM13
#                      and its reverse complement (--move --text-shortcuts tsc/c<N>.ell<ELL>t<TAU>.tsc;
#                      experiments/tsc_text_build.sh);
#   stpd               our STPD of the parse, plain parse, one occurrence per MEM (--csa --stpd-parse --no-intervals;
#                      MEMs checked, columns 1-3);
#   .csa: text-level steps on the RLCSA (--csa); .move: on the move structure (--move, c<N>.mv from index --move).
# O (default results/chr19/twolevel_ac) is the output directory; CHR19 (default ~/research/rlbwt-tricks/chr19) holds
# the indexes, blocks/queries.fa and the reference output blocks/q<N>s.blocks.tsv.
# Each round runs every configuration once, pinned to CPU $PIN (default 0), in an 11 GB cgroup without swap.  The MEMs
# and their intervals (columns 1-5) must equal the paper's BML output blocks/q<N>s.blocks.tsv.
# Logs in results/chr19/twolevel_ac/logs/q<N>.<config>.<alg>.<r>.log; table by experiments/twolevel_ac_table.py.
set -euo pipefail
cd "$(dirname "$0")/.."
NS=${1:-"33 65 129"}; CONFIGS=${2:-"rlcsa nojumps.csa nojumps.move unique.csa unique.move heavy08.csa heavy08.move"}; REP=${3:-3}
D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; Q=$D/blocks/queries.fa; O=${O:-results/chr19/twolevel_ac}; mkdir -p $O/logs
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"; TS="taskset -c ${PIN:-0}"
SCR=$(mktemp -d -p "${TMPDIR:-/tmp}" twolevel_ac.XXXX)
for n in $NS; do
  for r in $(seq 1 $REP); do
    for c in $CONFIGS; do
      t=${c#*.}; [ $c = rlcsa ] && t=csa; [ $c = stpd ] && t=csa; case $c in tsc*) t=move ;; esac
      cols=1-5
      case ${c%.*} in
        rlcsa)   X="$D/c$n" ;;
        nojumps) X="--twolevel --no-jumps $D/c$n" ;;
        unique)  X="--twolevel $D/unique/c${n}u" ;;
        heavy*|ref*) X="--twolevel --pshortcuts $D/psc/c$n.${c%.*}.psc $D/c$n" ;;
        tsc*)    X="--text-shortcuts $D/tsc/c$n.ell${c#tsc}.tsc $D/c$n" ;;
        stpd)    X="--stpd-parse --no-intervals $D/c$n"; cols=1-3 ;;
        *) echo "unknown config $c"; exit 1 ;;
      esac
      for a in bml fb; do
        F=""; [ $a = fb ] && F="--fb"
        log=$O/logs/q$n.$c.$a.$r.log
        echo "[$(date '+%F %T')] c$n $c $a repeat $r" >&2
        /usr/bin/time -f "wall %e s, peak %M KB" $CAP $TS bin/katka find -L 1000 --format mems --$t --stats -j 1 $F $X $Q $SCR/out.tsv > $log 2>&1
        cmp -s <(cut -f$cols $SCR/out.tsv) <(cut -f$cols $D/blocks/q${n}s.blocks.tsv) || { echo "c$n $c $a: MEMs or intervals differ from BML's"; exit 1; }
      done
    done
  done
done
rm -rf $SCR
