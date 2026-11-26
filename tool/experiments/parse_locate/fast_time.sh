#!/bin/bash
# Timing of locating from the parse alone, faster walks (2026-11-26), by the rules of Table tab:textless: one thread pinned
# to CPU $PIN (default 0), 11 GB cgroup without swap, huge pages (GLIBC_TUNABLES=glibc.malloc.hugetlb=1, as the paper's
# tables), loading excluded (find's "(total X s)"), median of $REP (default 3) rounds, L = 1000, BML and FB.
# Rows (as experiments/latency/final.sh): tl (two-level), tl+table+la2, tlur+table+la2 (▷, jump, tables and lookahead),
# tlh08+table (heavy paths θ = 80%, with the table).  Configurations of each row, all bin/katka-ploc2 unless said:
#   off    without --parse-locate;  on: with it (s = 64; 12 lanes, a run of rows at a time)
#   and on the rows tl and tlh08+table:  s32, s16 (the samples c<N>tl.s32.prs, .s16.prs)
#   and on tl only:  old (bin/katka-ploc0, 8912148: one row at a time, as it was timed), lanes0 ($KATKA_PLOC_LANES=0: one
#   row at a time, batched), rows ($KATKA_PLOC_RUNS=0: 12 lanes, a row at a time), nowalk ($KATKA_PLOC_NOWALK: the rows
#   collected but not walked)
# Within a round the configurations' order reverses from round to round.  Every output is compared with the row's first
# (off) output.  Logs to $OUT/time/, times to $OUT/time/times.txt (N row config alg round seconds peakKB).
set -euo pipefail
cd "$(dirname "$0")/../.."
D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; Q=$D/blocks/queries.fa; DW=${DW:-$HOME/diamond}; W=${W:-$HOME/diamond/ploc/fast}
OUT=${OUT:-results/chr19/parse_locate/fast}; REP=${REP:-3}; mkdir -p $W/ref $OUT/time
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"; TS="taskset -c ${PIN:-0}"; HP="GLIBC_TUNABLES=glibc.malloc.hugetlb=1"
ROWS=${ROWS:-"tl tl+table+la2 tlur+table+la2 tlh08+table"}
configs() { case $1 in tl) echo "off on s32 s16 old lanes0 rows nowalk" ;; tlh08+table) echo "off on s32 s16" ;; *) echo "off on" ;; esac; }
run1() {   # N row config alg round
  local N=$1 row=$2 c=$3 a=$4 r=$5 F=() E A bin=bin/katka-ploc2 P lg=$OUT/time/$1.$2.$3.$4.$5
  [ $a = fb ] && F=(--fb)
  case $row in
    tl) E="KATKA_TL_FB=1 KATKA_NO_FIRST_RUN=1"; A="--twolevel --no-jumps --textless" ;;
    tl+table+la2) E="KATKA_TL_FB=1 KATKA_LA_D=2"; A="--twolevel --no-jumps --textless" ;;
    tlur+table+la2) E="KATKA_TL_FB=1 KATKA_LA_D=2 KATKA_TL_RIGHT=1"; A="--twolevel --textless" ;;
    tlh08+table) E="KATKA_TL_FB=1"; A="--twolevel --pshortcuts $D/psc/c$N.heavy08.psc --textless" ;;
  esac
  local idx=$DW/c${N}tl; [ $row = tlur+table+la2 ] && idx=$DW/c${N}utl
  case $c in
    off) P="" ;; on|lanes0|rows|nowalk) P="--parse-locate $DW/c${N}tl.prs" ;; old) P="--parse-locate $DW/c${N}tl.prs"; bin=bin/katka-ploc0 ;;
    s32|s16) P="--parse-locate $DW/c${N}tl.$c.prs" ;;
  esac
  case $c in lanes0) E="$E KATKA_PLOC_LANES=0" ;; rows) E="$E KATKA_PLOC_RUNS=0" ;; nowalk) E="$E KATKA_PLOC_NOWALK=1" ;; esac
  /usr/bin/time -f "%M" -o $lg.peak $CAP $TS env $HP $E $bin find -L 1000 --format mems --stats -j 1 "${F[@]}" $A $P $idx $Q $W/o.tsv > $lg.log 2>&1
  local ref=$W/ref/$N.$row.$a.tsv
  [ -s $ref ] || cp $W/o.tsv $ref
  cmp -s $W/o.tsv $ref || { echo "== STOPPED: $N $row $c $a round $r: output differs from the row's first"; exit 1; }
  echo "$N $row $c $a $r $(grep -o '(total [0-9.]* s)' $lg.log | tr -d '()' | awk '{print $2}') $(cat $lg.peak)" | tee -a $OUT/time/times.txt >&2
}
echo "== start $(date '+%F %T'); up since $(uptime -s)" >&2
for N in ${NS:-33 65 129}; do
  for r in $(seq 1 $REP); do
    for row in $ROWS; do
      cs=$(configs $row); [ $((r % 2)) = 0 ] && cs=$(echo $cs | tr ' ' '\n' | tac | tr '\n' ' ')
      [ $r = 1 ] && cs="off ${cs/off /}"         # the reference output comes from off
      for c in $cs; do for a in bml fb; do run1 $N $row $c $a $r; done; done
    done
  done
done
rm -f $W/o.tsv
echo "== done $(date '+%F %T')" >&2
