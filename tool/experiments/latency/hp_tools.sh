#!/bin/bash
# Huge-page trial, step 5: GLIBC_TUNABLES=glibc.malloc.hugetlb=1 on ropebwt3 (3.10-r285, mem -t 1 -l 1000, BML) and
# Movi 2 (query --mem --min-mem-length 1000 --ftab-k 10 -t 1) at 16 individuals, with their October indexes and
# queries (copied from the external drive's twolevel_runs/twolevel/results/n16/ to $H); 3 rounds, with and without
# interleaved, one thread pinned to CPU 0, 11 GB cgroup.  ropebwt3's time as in run_ropebwt3.sh (realtime at
# "processed" minus at "loaded the BWT"); Movi 2's wall time from /usr/bin/time (it includes loading the index).
# Outputs with and without must be byte-identical.  Each run also samples the share of memory in huge pages.
set -euo pipefail
H=${H:-$HOME/data/latency/hp5}; cd $H
CAP="systemd-run --user --scope -q -p MemoryMax=11G -p MemorySwapMax=0"
qtime() { awk -F'::' '/loaded the BWT/ {split($3,a,"*"); l=a[1]} /processed/ {split($3,b,"*"); p=b[1]} END {printf "%.3f\n", p-l}' "$1"; }
share() { local p=$1 ah=0 rs=0 a b; while kill -0 $p 2>/dev/null; do
  if read a b < <(awk '/^Rss:/{r=$2} /^AnonHugePages:/{a=$2} END{print a+0, r+0}' /proc/$p/smaps_rollup 2>/dev/null); then
    [ "${b:-0}" -gt $rs ] && rs=$b; [ "${a:-0}" -gt $ah ] && ah=$a; fi; sleep 0.5; done
  awk -v a=$ah -v b=$rs 'BEGIN{printf "%.1f%% of %.0f MB", (b ? 100*a/b : 0), b/1024}'; }
for r in 1 2 3; do for hp in 0 1; do
  E="X=0"; [ $hp = 1 ] && E="GLIBC_TUNABLES=glibc.malloc.hugetlb=1"
  env $E $CAP taskset -c 0 ./rb3-bin mem -t 1 -l 1000 rb3/index.fmd rb3/queries.fa > rb3.$hp.out 2> rb3.$hp.$r.log & p=$!
  sleep 1; s=$(share $(pgrep -P $p -x rb3-bin || echo $p)); wait $p
  echo "ropebwt3 hp=$hp round $r: $(qtime rb3.$hp.$r.log) s, huge pages $s"
  env $E $CAP taskset -c 0 /usr/bin/time -f "%e" -o movi.$hp.$r.time ./movi-bin query -i movi -r rb3/queries.fa --mem --min-mem-length 1000 --ftab-k 10 --stdout -n -t 1 > movi.$hp.out 2> movi.$hp.$r.log & p=$!
  sleep 2; s=$(share $(pgrep -n -x movi-regular || echo $p)); wait $p   # the wrapper runs bin/movi-regular (comm "movi-regular"); the first run probed the wrapper
  echo "Movi 2 hp=$hp round $r: $(cat movi.$hp.$r.time) s wall, huge pages $s"
done
cmp -s rb3.0.out rb3.1.out && cmp -s movi.0.out movi.1.out && echo "round $r: outputs with and without identical" || { echo "== STOPPED: outputs differ"; exit 1; }
done
echo "== done"
