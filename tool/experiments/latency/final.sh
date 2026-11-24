#!/bin/bash
# One final timing session for every timed row of "More RLBWT Tricks" (arXiv v3, Tables 1-9) and the latency study's
# new rows, without and with huge pages (GLIBC_TUNABLES=glibc.malloc.hugetlb=1), the two modes interleaved row by row.
#   experiments/latency/final.sh ["NS"] [REP]          NS default "33 65 129" (16, 32, 64 individuals), REP default 3
# Rules as the paper's: one thread pinned to CPU $PIN (default 0), 11 GB cgroup without swap, load time excluded,
# median of REP rounds.  Each round runs every row once in both modes; the mode that goes first alternates from row to
# row and from round to round.  A row whose REP times without huge pages differ by more than 3% gets two more runs
# without huge pages (median of five, marked in the table).
# Times: katka find's "(total X s)" (experiments/twolevel_ac.sh, twolevel_ac_table.py); ropebwt3's "processed" minus
# "loaded the BWT" (run_ropebwt3.sh); Movi 2's "processing the reads" (run_movi_final.sh); the STPD-index driver's
# "finding them" (stpd_index_rerun.sh).
# Rows (ROWS overrides, ALGS overrides each row's algorithms, for tests):
#   katka-base, as the paper's scripts: rlcsa, move (plain search on the RLCSA, on the move structure alone);
#     nojumps.csa, nojumps.move (two-level); stpd (our STPD of the parse, RLCSA); tsc25t0, tsc100t0, tsc25t6
#     (almost-funnels on the text); ref4, ref16, ref64, heavy08, heavy09, heavy095, unique (two-level on the move
#     structure: almost-funnels from CHM13's parse and along heavy paths, the ▷ encoding); unitig (the ◇ encoding,
#     not in the default ROWS; its indexes in $DW, default ~/diamond, built by index --twolevel --continuation unitig --blocks-from c<N>.lvl);
#   textless, textless+table+la2 (bin/katka-tl, BML only; MEMs in columns 1-3, occurrences instead of the interval):
#     two-level search without the index of the text (src/textless.hpp), plain and with the table and lookahead d = 2;
#   tl, tlu, tlur, tlw, tlh08, tlh09, tlh095, tlr4, tlr16, tlr64 (+table, +table+la2; bin/katka-tl2, BML and FB): stage 2,
#     part C, without the index of the text, minimal widths: plain, ▷, ▷ with the right-end jump, ◇, heavy paths, almost-
#     funnels from CHM13's parse (c<N>tl, c<N>utl, c<N>wtl in $DW);
#   new: nojumps.move+table (bin/katka-table), nojumps.move+table+la2 (bin/katka-la, d = 2), nojumps.move+la4
#     (bin/katka-la without the table, d = 4); unique+table, unique+table+la2; heavy08/09/095+table;
#   move-structure latency, part 2 (bin/katka-chop): move+chop (plain search chopped into 64 pieces, one by one),
#     move+co<K> (the 64 pieces interleaved in K coroutines);
#   ropebwt3 (3.10-r285, mem -t 1 -l 1000; FB = --old-mem), movi2 (9bc0acc, query --mem --ftab-k 10 -t 1,
#     --min-mem-length 1000; "all" = 1, its own query for all MEMs), stpdindex (Cenzato et al.'s, RLZ + r-index,
#     bin/stpd-index-mems [--fb]; 16 and 32 individuals only), with the October indexes in $T/n<16|32|64>/.
# Checks: every katka output byte-identical to katka-base's for the same row, size and algorithm, and its MEMs and
# intervals (columns 1-5; 1-3 for stpd and stpdindex) equal to the paper's blocks/q<N>s.blocks.tsv; every ropebwt3 and
# Movi 2 output byte-identical to its first run.
# Huge pages: during every run (both modes) a probe pinned to CPU $PROBE (default 5, another core) reads the
# process's smaps_rollup every 2 s; it logs the largest AnonHugePages and Rss (for Movi 2, of the worker
# movi-regular).  Logs in $W/logs/q<N>.<row>.<alg>.<off|hp>.<r>.{log,time,hp}.
set -euo pipefail
cd "$(dirname "$0")/../.."
NS=${1:-"33 65 129"}; REP=${2:-3}
D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; Q=$D/blocks/queries.fa
W=${W:-$HOME/data/latency/final}; T=${T:-$W/tools}; H=${H:-$HOME/data/latency/hp5}; O=$W/logs; mkdir -p $O $W/ref $W/out
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"; PIN=${PIN:-0}; PROBE=${PROBE:-5}
HPENV="GLIBC_TUNABLES=glibc.malloc.hugetlb=1"
ROWS=${ROWS:-"rlcsa move nojumps.csa nojumps.move nojumps.move+table nojumps.move+table+la2 nojumps.move+la4 stpd
  tsc25t0 tsc100t0 tsc25t6 ref4 ref16 ref64 heavy08 heavy08+table heavy09 heavy09+table heavy095 heavy095+table
  unique unique+table unique+table+la2 ropebwt3 movi2 stpdindex"}
ind() { echo $(( ($1 - 1) / 2 )); }
algs() { [ -n "${ALGS:-}" ] && { echo $ALGS; return; }; case $1 in movi2) echo "bml all" ;; textless*) echo "bml" ;; *) echo "bml fb" ;; esac; }
kargs() {  # katka find's options for a base row at size $2
  case $1 in
    rlcsa) echo "--csa $D/c$2" ;; move) echo "--move $D/c$2" ;;
    nojumps.csa) echo "--csa --twolevel --no-jumps $D/c$2" ;; nojumps.move) echo "--move --twolevel --no-jumps $D/c$2" ;;
    stpd) echo "--csa --stpd-parse --no-intervals $D/c$2" ;;
    tsc*) echo "--move --text-shortcuts $D/tsc/c$2.ell${1#tsc}.tsc $D/c$2" ;;
    ref*|heavy*) echo "--move --twolevel --pshortcuts $D/psc/c$2.$1.psc $D/c$2" ;;
    unique) echo "--move --twolevel $D/unique/c${2}u" ;;
    unitig) echo "--move --twolevel ${DW:-$HOME/diamond}/c${2}w" ;;   # ◇ (index --continuation unitig)
    textless) echo "--twolevel --no-jumps --textless ${DW:-$HOME/diamond}/c$2" ;;   # without the index of the text (experiments/textless/check.sh builds .tlx)
    tl) echo "--twolevel --no-jumps --textless ${DW:-$HOME/diamond}/c${2}tl" ;;     # stage 2, part C: c<N>tl.lvl + .tl2 (minimal widths)
    tlu|tlur) echo "--twolevel --textless ${DW:-$HOME/diamond}/c${2}utl" ;;          #   ▷ (tlur: the right-end jump)
    tlw) echo "--twolevel --textless ${DW:-$HOME/diamond}/c${2}wtl" ;;               #   ◇
    tlh08|tlh09|tlh095|tlr4|tlr16|tlr64) local p=${1#tl}; p=${p/h/heavy}; p=${p/r/ref}; echo "--twolevel --pshortcuts $D/psc/c$2.$p.psc --textless ${DW:-$HOME/diamond}/c${2}tl" ;;
    *) echo "unknown row $1" >&2; exit 1 ;;
  esac; }
kbin() {  # binary and environment of a row's variant
  case $1 in textless) echo "bin/katka-tl KATKA_NO_FIRST_RUN=1"; return ;; textless+table+la2) echo "bin/katka-tl KATKA_LA_D=2"; return ;; esac
  case $1 in tl*) local r=""; [ ${1%%+*} = tlur ] && r=" KATKA_TL_RIGHT=1"
    case ${1#*+} in $1) echo "bin/katka-tl2 KATKA_TL_FB=1 KATKA_NO_FIRST_RUN=1$r" ;; table) echo "bin/katka-tl2 KATKA_TL_FB=1$r" ;; table+la2) echo "bin/katka-tl2 KATKA_TL_FB=1 KATKA_LA_D=2$r" ;; esac; return ;; esac
  case ${1#*+} in
    $1) echo "bin/katka-base" ;; table) echo "bin/katka-table" ;; table+la2) echo "bin/katka-la KATKA_LA_D=2" ;;
    la4) echo "bin/katka-la KATKA_NO_FIRST_RUN=1 KATKA_LA_D=4" ;;
    chop) echo "bin/katka-chop KATKA_CHOP=64" ;; co[0-9]*) echo "bin/katka-chop KATKA_CHOP=64 KATKA_CO_K=${1#*+co}" ;;
    *) echo "unknown variant $1" >&2; exit 1 ;;
  esac; }
tval() {  # the time of a run from its log
  case $1 in
    ropebwt3) awk -F'::' '/loaded the BWT/ {split($3,a,"*"); l=a[1]} /processed/ {split($3,b,"*"); p=b[1]} END {printf "%.3f\n", p-l}' $2 ;;
    movi2) sed 's/\x1b\[[0-9;]*m//g' $2 | grep -o 'processing the reads: [0-9.]*' | awk '{print $4}' ;;
    stpdindex) grep -o 'finding them [0-9.]* s' $2 | awk '{print $3}' ;;
    *) grep -o '(total [0-9.]* s)' $2 | tr -d '()' | awk '{print $2}' ;;
  esac; }
probe() {  # largest AnonHugePages and Rss of the newest process named $1 while $2 runs
  taskset -cp $PROBE $BASHPID > /dev/null; local q="" ah=0 rs=0 a b
  while kill -0 $2 2>/dev/null; do
    [ -z "$q" ] && q=$(pgrep -n -x "$1" || true)
    if [ -n "$q" ] && read a b < <(awk '/^Rss:/{r=$2} /^AnonHugePages:/{h=$2} END{print h+0, r+0}' /proc/$q/smaps_rollup 2>/dev/null); then
      [ "${b:-0}" -gt $rs ] && rs=$b; [ "${a:-0}" -gt $ah ] && ah=$a; fi
    sleep 2; done
  awk -v a=$ah -v b=$rs 'BEGIN{printf "huge pages %.1f%% of %.0f MB resident (AnonHugePages %d KB, Rss %d KB)\n", (b ? 100*a/b : 0), b/1024, a, b}'; }
run1() {  # size, row, algorithm, mode, round
  local n=$1 row=$2 a=$3 m=$4 r=$5 E="X=0" cwd=$PWD comm out cols=1-5; [ $m = hp ] && E=$HPENV
  local L=$O/q$n.$row.$a.$m.$r; out=$W/out/q$n.$row.$a.tsv; local cmd
  case $row in
    ropebwt3) comm=rb3-bin; F="-l 1000"; [ $a = fb ] && F="--old-mem -l 1000"
      cmd="$H/rb3-bin mem -t 1 $F $T/n$(ind $n)/rb3/index.fmd $H/rb3/queries.fa" ;;
    movi2) comm=movi-regular; cwd=$H; F=1000; [ $a = all ] && F=1
      cmd="./movi-bin query -i $T/n$(ind $n)/movi -r $H/rb3/queries.fa --mem --min-mem-length $F --ftab-k 10 --stdout -n -t 1" ;;
    stpdindex) comm=stpd-index-mems; F=""; [ $a = fb ] && F="--fb"; cols=1-3
      cmd="$PWD/bin/stpd-index-mems $D/stpd$n/st.RLZ-r-index.stpdi $Q 1000 $F" ;;
    *) local b=$(kbin $row); local bin=${b%% *}; comm=$(basename $bin); [ "$b" != "$bin" ] && E="$E ${b#* }"
      F=""; [ $a = fb ] && F="--fb"; [ ${row%%+*} = stpd ] && cols=1-3; case ${row%%+*} in textless|tl*) cols=1-3 ;; esac   # (no intervals: occurrences)
      cmd="$bin find -L 1000 --format mems --stats -j 1 $F $(kargs ${row%%+*} $n) $Q $out" ;;
  esac
  echo "[$(date '+%F %T')] c$n $row $a $m round $r" >&2
  case $row in
    ropebwt3|movi2|stpdindex) (cd $cwd && exec /usr/bin/time -f "wall %e s, peak %M KB" -o $L.time $CAP taskset -c $PIN env $E $cmd > $out 2> $L.log) & ;;
    *) /usr/bin/time -f "wall %e s, peak %M KB" -o $L.time $CAP taskset -c $PIN env $E $cmd > $L.log 2>&1 & ;;
  esac
  local p=$!; probe $comm $p > $L.hp & local pp=$!
  wait $p || { echo "== STOPPED: c$n $row $a $m round $r exited with $? (see $L.log)"; exit 1; }
  wait $pp
  local ref=$W/ref/q$n.${row%%+*}.$a.tsv
  if [ ! -s $ref ]; then      # the first run of a base row is the reference; katka's MEMs must be the paper's
    [ $row = ${row%%+*} ] || { echo "== STOPPED: no reference for c$n $row $a (its base row must run first)"; exit 1; }
    case $row in ropebwt3|movi2) ;; *) cmp -s <(cut -f$cols $out) <(cut -f$cols $D/blocks/q${n}s.blocks.tsv) ||
      { echo "== STOPPED: c$n $row $a: MEMs differ from the paper's"; exit 1; } ;; esac
    cp $out $ref
  fi
  cmp -s $out $ref || { echo "== STOPPED: c$n $row $a $m round $r: output differs from the reference"; exit 1; }
  echo "  $(tval $row $L.log) s; $(cat $L.hp)" >&2
}
echo "== final start $(date '+%F %T'): NS '$NS', REP $REP; up since $(uptime -s); $(uptime | sed 's/.*load/load/')"
for n in $NS; do
  rows=$ROWS; [ $n = 129 ] && rows=$(echo $ROWS | tr ' ' '\n' | grep -vx stpdindex | tr '\n' ' ')
  for r in $(seq 1 $REP); do
    i=0; for row in $rows; do
      i=$((i + 1)); modes="off hp"; [ $(( (i + r) % 2 )) = 1 ] && modes="hp off"
      for m in $modes; do for a in $(algs $row); do run1 $n $row $a $m $r; done; done
    done
  done
  for row in $rows; do for a in $(algs $row); do      # unstable without huge pages: two more runs
    s=$(for r in $(seq 1 $REP); do tval $row $O/q$n.$row.$a.off.$r.log; done | sort -g | awk 'NR==1{lo=$1} {hi=$1} END{d=(hi-lo)/lo; print (d > 0.03 ? 1 : 0)}')
    [ $s = 1 ] && for r in $(seq $((REP + 1)) $((REP + 2))); do run1 $n $row $a off $r; done
  done; done
  echo "-- c$n done $(date '+%F %T')"
done
echo "== final done $(date '+%F %T')"
