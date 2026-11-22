#!/bin/bash
# Huge-page trial, besides the timings (time.sh hp1-hp3): (1) the share of the process's memory in huge pages
# (AnonHugePages / Rss from /proc/PID/smaps_rollup, the largest values sampled every 0.5 s), 3 runs per row, variant and
# size, with GLIBC_TUNABLES=glibc.malloc.hugetlb=1; (2) at 16 individuals, page walks (dtlb_load_misses.walk_active) as a
# share of the search's cycles, with and without, the 100-character run subtracted as in b_perf.sh.  Untimed.
set -euo pipefail
cd "$(dirname "$0")/../.."
D=${CHR19:-$HOME/research/rlbwt-tricks/chr19}; Q=$D/blocks/queries.fa; W=${W:-$HOME/data/latency/hp}; mkdir -p $W
HP=glibc.malloc.hugetlb=1
args() { case $1 in nojumps) echo "--twolevel --no-jumps $D/c$2" ;; unique) echo "--twolevel $D/unique/c${2}u" ;;
         heavy09) echo "--twolevel --pshortcuts $D/psc/c$2.heavy09.psc $D/c$2" ;; move) echo "$D/c$2" ;; esac; }
VARS="nojumps:current:bin/katka-base:0 nojumps:table:bin/katka-table:0 nojumps:la2:bin/katka-la:2 unique:current:bin/katka-base:0
      unique:table:bin/katka-table:0 unique:la2:bin/katka-la:2 heavy09:current:bin/katka-base:0 heavy09:table:bin/katka-table:0 move:current:bin/katka-base:0"
echo "row variant size run AnonHugePages_kB Rss_kB share" > $W/share.txt
for n in 33 65 129; do for v in $VARS; do IFS=: read row lab bin d <<< "$v"
  for r in 1 2 3; do
    GLIBC_TUNABLES=$HP KATKA_LA_D=$d taskset -c 0 $bin find -L 1000 --format mems --move --stats -j 1 $(args $row $n) $Q $W/out.tsv > $W/probe.log 2>&1 & p=$!
    ah=0; rs=0
    while kill -0 $p 2>/dev/null; do
      if read a b < <(awk '/^Rss:/{r=$2} /^AnonHugePages:/{a=$2} END{print a+0, r+0}' /proc/$p/smaps_rollup 2>/dev/null); then
        [ "${b:-0}" -gt $rs ] && rs=$b; [ "${a:-0}" -gt $ah ] && ah=$a; fi
      sleep 0.5
    done; wait $p
    cmp -s $W/out.tsv $HOME/data/latency/time/ref/q$n.$row.bml.tsv || { echo "== STOPPED: $row $lab $n differs"; exit 1; }
    echo "$row $lab $n $r $ah $rs $(awk -v a=$ah -v b=$rs 'BEGIN{printf "%.1f%%", 100*a/b}')" >> $W/share.txt
  done
done; done
G=cycles,instructions,{dtlb_load_misses.walk_active,dtlb_load_misses.miss_causes_a_walk,cycle_activity.stalls_l3_miss}
echo "row variant hp walk_share_of_cycles walks_per_1000_instructions l3_stall_share" > $W/walks.txt
for v in $VARS; do IFS=: read row lab bin d <<< "$v"; for hp in 0 1; do
  E="KATKA_LA_D=$d"; [ $hp = 1 ] && E="$E GLIBC_TUNABLES=$HP"
  env $E taskset -c 0 perf stat -x, -o $W/w.full.csv -e "$G" -- $bin find -L 1000 --format mems --move --stats -j 1 $(args $row 33) $Q $W/out.tsv > /dev/null 2>&1
  cmp -s $W/out.tsv $HOME/data/latency/time/ref/q33.$row.bml.tsv || { echo "== STOPPED: walks $row $lab differs"; exit 1; }
  env $E taskset -c 0 perf stat -x, -o $W/w.load.csv -e "$G" -- $bin find -L 1000 --format mems --move --stats -j 1 $(args $row 33) $W/../b/q100.fa $W/o100.tsv > /dev/null 2>&1
  cp $W/w.full.csv $W/walks.$row.$lab.hp$hp.full.csv; cp $W/w.load.csv $W/walks.$row.$lab.hp$hp.load.csv
  python3 - $W/w.full.csv $W/w.load.csv "$row $lab $hp" >> $W/walks.txt <<'PY'
import sys
def rd(f):
    d = {}
    for l in open(f):
        x = l.strip().split(",")
        if len(x) > 4 and x[0].isdigit(): d[x[2].replace(":u", "")] = int(x[0])
    return d
a, b = rd(sys.argv[1]), rd(sys.argv[2]); s = {k: a[k] - b[k] for k in a}
print(sys.argv[3], f"{100 * s['dtlb_load_misses.walk_active'] / s['cycles']:.1f}%", f"{1000 * s['dtlb_load_misses.miss_causes_a_walk'] / s['instructions']:.2f}", f"{100 * s['cycle_activity.stalls_l3_miss'] / s['cycles']:.1f}%")
PY
done; done
rm -f $W/out.tsv $W/o100.tsv $W/w.full.csv $W/w.load.csv $W/probe.log
echo "== done"
