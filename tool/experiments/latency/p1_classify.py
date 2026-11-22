#!/usr/bin/env python3
# Latency hiding on the move structure, part 1, step 5: PEBS samples of mem_load_retired.l3_miss (perf record -d) of
# plain search on the move structure, classified by load site.
#   python3 p1_classify.py BINARY PERF.DATA SRCDIR [IPMAP.tsv]
# Each sample's instruction is expanded by `addr2line -i` into its inline chain; the class is decided by the innermost
# frame in move.hpp (move_index), read in SRCDIR (a copy of the sources BINARY was built from):
#   the LF step, `while (o >= len(q))` (move.hpp's lf): the destination row (the first record read after q = dest(q))
#     or a row read while fast-forwarding (the records after it).  These are one source line, so IPMAP (ip, class)
#     tells the two load instructions apart, from the disassembly; without it they are reported together;
#   `o += off(q); q = dest(q)`: the record of the row the step starts from;
#   sym(a), sym(e): the records at the interval's ends, checked for the character;
#   the scans of the neighbouring runs, runs_of's Elias-Fano lists and len(b): repositioning an end of the interval
#     when the character does not match;
#   at(), rows(): converting between rows and cursors (starts' Elias-Fano list);
#   the 8-mer table (mems.hpp, M.tab[key]); anything else, by function.
import sys, subprocess, re, collections, os
BIN, DATA, SRC = sys.argv[1:4]
IPMAP = {}
if len(sys.argv) > 4:
    for l in open(sys.argv[4]):
        if l.strip() and not l.startswith("#"): a, c = l.rstrip("\n").split("\t")[:2]; IPMAP[int(a, 16)] = c
out = subprocess.run(["perf", "script", "-i", DATA, "--show-mmap-events", "-F", "ip,dso,addr"], capture_output=True, text=True).stdout
base = None; binname = os.path.basename(BIN); samples = collections.Counter(); total = 0
for l in out.splitlines():
    if "PERF_RECORD_MMAP" in l:
        if l.rstrip().endswith(binname) and " r-xp " in l:
            m = re.search(r"\[0x([0-9a-f]+)\(0x[0-9a-f]+\) @ 0x([0-9a-f]+)", l); base = (int(m.group(1), 16), int(m.group(2), 16))
        continue
    m = re.match(r"\s*([0-9a-f]+)\s+([0-9a-f]+)\s+\((.*)\)\s*$", l)
    if not m: continue
    total += 1
    ip, dso = int(m.group(2), 16), m.group(3)
    samples[(ip - base[0] + base[1]) if dso.endswith(binname) and base else os.path.basename(dso)] += 1
addrs = sorted(a for a in samples if isinstance(a, int))
a2l = subprocess.run(["addr2line", "-i", "-f", "-C", "-a", "-e", BIN], input="\n".join(hex(a) for a in addrs), capture_output=True, text=True).stdout.splitlines()
chains = {}; k = 0; cur = None
while k < len(a2l):
    l = a2l[k]
    if l.startswith("0x"): cur = int(l, 16); chains[cur] = []; k += 1; continue
    fn, loc = l, a2l[k + 1] if k + 1 < len(a2l) else "??:0"; k += 2
    m = re.match(r"(.*):(\d+)", loc.split(" (discriminator")[0])
    chains[cur].append((fn, os.path.basename(m.group(1)) if m else loc, int(m.group(2)) if m else 0))
_src = {}
def text(f, n):
    p = os.path.join(SRC, f)
    if p not in _src: _src[p] = open(p).read().splitlines() if os.path.exists(p) else []
    s = _src[p]; return s[n - 1].strip() if 0 < n <= len(s) else ""
def classify(ip):
    ch = chains.get(ip, [])
    J = " | ".join(f for f, _, _ in ch)
    if re.search(r"::load\b|::load\(|\brvec\b|open_or_die|make_table", J): return "load (excluded)"
    for f, fl, ln in ch:
        if fl == "move.hpp" and not re.search(r"move_index::(len|sym|off|dest|code2)\b", f):   # the accessors' callers
            t = text(fl, ln)
            if "while (o >= len(q))" in t: return IPMAP.get(ip, "the LF step: destination row or fast-forward")
            if "q = dest(q)" in t: return "the record of the row the step starts from (off, dest)"
            if "sym(a) != s" in t and "if" in t and "while" not in t: return "the record at the interval's start (sym(a))"
            if "sym(e) != s" in t and "if" in t and "while" not in t: return "the record at the interval's end (sym(e))"
            if "while (k < SCAN" in t: return "repositioning: scanning the neighbouring runs"
            if "runs_of" in t: return "repositioning: runs_of (Elias-Fano)"
            if "oe = len(b)" in t or "sym(b) != s" in t: return "repositioning: the new end's record"
            if "starts." in t: return "rows <-> cursors (starts)"
            return f"move.hpp:{ln} {t[:50]}"
    for f, fl, ln in ch:
        if fl == "mems.hpp" and "M.tab[key]" in text(fl, ln): return "the 8-mer table"
    return "other (" + (ch[-1][0] if ch else "?")[:60] + ")"
cls = collections.Counter(); lf_ips = collections.Counter()
for a, c in samples.items():
    if not isinstance(a, int): cls["outside katka: " + a] += c; continue
    k1 = classify(a); cls[k1] += c
    if k1.startswith("the LF step") or a in IPMAP: lf_ips[a] += c
load = cls.pop("load (excluded)", 0); T = total - load
print(f"samples {total}, of which load {load}; shares of the {T} others")
for k, c in sorted(cls.items(), key=lambda x: -x[1]): print(f"  {100.0 * c / T:6.2f}%  {k}")
print("instructions of the LF step's `while (o >= len(q))` line:")
for a, c in sorted(lf_ips.items(), key=lambda x: -x[1]): print(f"  {hex(a)}  {100.0 * c / T:6.2f}%  {IPMAP.get(a, '')}")
