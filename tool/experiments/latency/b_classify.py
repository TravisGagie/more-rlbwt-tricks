#!/usr/bin/env python3
# Classify perf record samples of katka by inline chain (B of the latency study).
#   python3 b_classify.py BINARY PERF.DATA SRCDIR [LINES.tsv]
# With B_SITES=1, also the samples of each class by their call site in search2 (the line of the outermost search2
# frame, not its lambdas), or the outermost function when the chain has no search2 frame.
# With B_STRICT=1, the regular expressions below see each frame's function name without its parameter lists and
# template arguments.  Without it (as in the class files first published, b/*.class.txt), a frame in search2 matches
# "stepper" through search2's own parameter list (katka::stepper const&), so every sample in search2 outside
# EFRank::lf2 counts as "text index (move structure)": the pair symbols, B rank and select, BS and the rest.
# The character steps are then the frames of move_index, rlcsa and stepper, and search2's char_steps_to lambda
# ({lambda(long)#1}: lambdas are numbered per signature).  The lookahead's prefetches (EFRank::pf_first, pf_runs)
# get a class of their own, so the parse's index and X are as without B_STRICT.  Samples in search_textless
# (textless.hpp) outside the parse's BWT go by the line of its outermost frame: the right end (lookup), the parse steps
# or the left end (comparisons), split at the comments starting "// parse steps" and "// the left end"; the shortcuts
# over phrases (parse_shortcuts) inside it are their own class.
# Each sample in BINARY is mapped to its address in the file and expanded by `addr2line -i -f -C` into its inline chain
# (innermost first).  SRCDIR holds the sources BINARY was built from (a copy taken at build time), so that a frame's
# source line can be read: the class of a load in the parse's index is decided by the text of the lines in EFRank::lf2
# and EFRank::lf_one that inline it, and pbwt / bbwt by the text of the line in search2 that calls lf2.
# Classes in the parse's index (pbwt; bbwt in the encoding row):
#   (a) finding c's first run: KS.count_less(c n), the check that c occurs (KS.access(b)), FP.access(b), and, with the
#       first-run table, the table's own loads;
#   (b) other loads whose address depends on c only;
#   (c1) the predecessor searches for c n + sp and c n + ep (KS.count_less in lf_one);
#   (c2) what follows them: KS.access(j), FP.access2(j) and the arithmetic of lf_one;
#   lookahead: the prefetches of the lookahead build.
import sys, subprocess, re, collections, os
BIN, DATA, SRC = sys.argv[1:4]
out = subprocess.run(["perf", "script", "-i", DATA, "--show-mmap-events", "--no-inline", "-F", "ip,dso"], capture_output=True, text=True).stdout
# samples: blocks of "ip (dso)" lines, the leaf first and then the return addresses (with --call-graph lbr); without
# call graphs one line per sample
base = None; binname = os.path.basename(BIN); samples = collections.Counter(); cur = []; total = 0
def flush():
    global cur, total
    if cur: samples[tuple(cur)] += 1; total += 1
    cur = []
for l in out.splitlines():
    if "PERF_RECORD_MMAP" in l:
        if l.rstrip().endswith(binname) and " r-xp " in l:
            m = re.search(r"\[0x([0-9a-f]+)\(0x[0-9a-f]+\) @ 0x([0-9a-f]+)", l); base = (int(m.group(1), 16), int(m.group(2), 16))
        continue
    m = re.match(r"(\s*)([0-9a-f]+)\s+\((.*)\)\s*$", l)
    if not m: flush(); continue
    if not m.group(1) and cur: flush()            # a sample without a call chain: one unindented line each
    ip, dso = int(m.group(2), 16), m.group(3)
    cur.append((ip - base[0] + base[1], None) if dso.endswith(binname) and base else (0, os.path.basename(dso)))
flush()
want = set()
for sm in samples:
    for k, (a, d) in enumerate(sm):
        if d is None: want.add(a if k == 0 else a - 1)
addrs = sorted(want)
a2l = subprocess.run(["addr2line", "-i", "-f", "-C", "-a", "-e", BIN], input="\n".join(hex(a) for a in addrs), capture_output=True, text=True).stdout.splitlines()
chains = {}; curk = None
k = 0
while k < len(a2l):
    l = a2l[k]
    if l.startswith("0x"): curk = int(l, 16); chains[curk] = []; k += 1; continue
    fn, loc = l, a2l[k + 1] if k + 1 < len(a2l) else "??:0"; k += 2
    m = re.match(r"(.*):(\d+)", loc.split(" (discriminator")[0])
    chains[curk].append((fn, m.group(1) if m else loc, int(m.group(2)) if m else 0))
def full_chain(sm):
    ch = []
    for k, (a, d) in enumerate(sm):
        if d is None: ch += chains.get(a if k == 0 else a - 1, [])
        else: ch.append((d, d, 0))
    return ch
_src = {}
def text(f, n):
    p = os.path.join(SRC, os.path.basename(f))
    if p not in _src: _src[p] = open(p).read().splitlines() if os.path.exists(p) else []
    s = _src[p]; return s[n - 1] if 0 < n <= len(s) else ""
def strip(f):                                       # a function name without parameter lists and template arguments
    out, d = [], 0
    for x in re.sub(r"\{lambda\(([^)]*)\)#(\d+)\}", r"{lambda[\1]#\2}", f):
        if x in "(<": d += 1
        elif x in ")>": d = max(0, d - 1)
        elif d == 0: out.append(x)
    return "".join(out)
STRICT = bool(os.environ.get("B_STRICT"))
_tlm = {}
def _textless_marks(f):                             # the lines of "// parse steps" and "// the left end" in textless.hpp
    if f not in _tlm:
        p = os.path.join(SRC, os.path.basename(f)); lines = open(p).read().splitlines() if os.path.exists(p) else []
        a = [i + 1 for i, l in enumerate(lines) if l.strip().startswith("// parse steps")]; b = [i + 1 for i, l in enumerate(lines) if l.strip().startswith("// the left end")]
        _tlm[f] = (a[0], b[0]) if a and b else None
    return _tlm[f]
def classify(ch):
    names = [strip(f) if STRICT else f for f, _, _ in ch]
    J = " | ".join(names)
    if re.search(r"::load\b|::load\(|build_fine|\brvec\b|\bload_|::prepare|open_or_die", J): return "load (excluded)", None
    for idx, (f, fl, ln) in enumerate(ch):
        if "EFRank::lf2" in f or "EFRank::lf_one" in f:
            # the caller of lf2 (the first frame outside EFRank after it) decides pbwt or bbwt
            j = idx
            while j < len(ch) and "EFRank" in ch[j][0]: j += 1
            caller = text(ch[j][1], ch[j][2]) if j < len(ch) else ""
            which = "bbwt" if "bbwt" in caller else ("pbwt" if "pbwt" in caller else "EFRank, other caller")
            # the lines inside lf_one / lf2 that inline this sample
            sub = None
            for f2, fl2, ln2 in ch:
                t = text(fl2, ln2)
                if "LATPF" in t: sub = "lookahead"; break
                if "EFRank::lf_one" in f2:
                    if "count_less(base +" in t: sub = "(c1) predecessor searches for c n + sp, c n + ep"
                    elif "FP.access(b)" in t or "LATFIRST" in t: sub = "(a) finding c's first run"
                    else: sub = "(c2) KS.access(j), FP.access2(j), arithmetic"
                    break
                if "EFRank::lf2" in f2:
                    if "count_less(base)" in t or "KS.access(b)" in t or "LATFIRST" in t or "ft[c]" in t: sub = "(a) finding c's first run"
                    elif "lf_one" in t: sub = "(c) lf_one"
                    else: sub = "(b) other, depends on c only"
                    break
            return "parse's index: " + which, sub
    if STRICT and re.search(r"EFRank::pf_", J): return "the lookahead's prefetches (parse's index)", None
    if STRICT and "search_textless" in J and "parse_shortcuts" in J: return "shortcuts over phrases", None
    if STRICT and "search_textless" in J:           # two-level search without the index of the text (textless.hpp), by the
        fr = [x for x in ch if "search_textless" in x[0] and "lambda" not in x[0]]   # line of its outermost frame
        if fr:
            src = _textless_marks(fr[-1][1])
            ln = fr[-1][2]
            if src and ln >= src[1]: return "textless: the left end (comparisons)", None
            if src and ln >= src[0]: return "textless: parse steps, other", None
            return "textless: the right end (lookup)", None
    if STRICT and re.search(r"search2::\{lambda\[long\]#1\}", J): return "text index (move structure)", None
    if STRICT and re.search(r"two_level::sym\b", J): return "block bookkeeping (BS, symbols)", None
    if re.search(r"move_index|rlcsa|stepper|RLCSA", J): return "text index (move structure)", None
    if re.search(r"parse_pattern|for_each_syncmer|pattern_parse|PhraseDict|phrase_hash", J): return "pattern parsing", None
    if re.search(r"parse_shortcuts", J): return "shortcuts over phrases", None
    if re.search(r"RLBits", J): return "entering and leaving the parse (B)", None
    if re.search(r"BlockStarts|two_level::sym|two_level::succ|two_level::can_continue", J): return "block bookkeeping (BS, symbols)", None
    if re.search(r"read_fasta|fasta|revcomp|complement", J, re.I): return "reading queries, reverse complement", None
    if "search2" in J: return "search2, other", None
    return "other in katka (" + (names[-1] if names else "?") + ")", None
cls = collections.Counter(); subc = collections.Counter(); lines = collections.Counter(); sites = collections.Counter()
for sm, c in samples.items():
    if sm[0][1] is not None:                       # the leaf is outside katka
        cls["outside katka: " + sm[0][1]] += c; continue
    ch = full_chain(sm)
    k1, k2 = classify(ch)
    cls[k1] += c
    if k2: subc[(k1, k2)] += c
    if os.environ.get("B_SITES"):
        fr = [x for x in ch if "search2" in x[0] and "lambda" not in x[0]]
        site = (f"search2 {os.path.basename(fr[-1][1])}:{fr[-1][2]}  {text(fr[-1][1], fr[-1][2]).strip()[:80]}" if fr
                else "outside search2: " + (re.sub(r"\(.*", "", ch[-1][0])[:80] if ch else "?"))
        sites[(k1, site)] += c
    if k1.startswith("parse's index") and ch: lines[(k1, os.path.basename(ch[0][1]) + ":" + str(ch[0][2]), text(ch[0][1], ch[0][2]).strip()[:70])] += c
load = cls.get("load (excluded)", 0); T = total - load
print(f"samples {total}, of which load {load}; shares below are of the {T} others")
for k, c in sorted(cls.items(), key=lambda x: -x[1]):
    if k != "load (excluded)": print(f"  {100.0 * c / T:6.2f}%  {k}")
print("parse's index by class:")
for (k1, k2), c in sorted(subc.items()): print(f"  {100.0 * c / T:6.2f}%  {k1} {k2}")
print("parse's index, top source lines (innermost):")
for (k1, ln, t), c in sorted(lines.items(), key=lambda x: -x[1])[:20]: print(f"  {100.0 * c / T:6.2f}%  {k1.split(': ')[1]} {ln}  {t}")
if len(sys.argv) > 4:                                # every innermost line in the parse's index, as TSV
    with open(sys.argv[4], "w") as o:
        for (k1, ln, t), c in sorted(lines.items(), key=lambda x: -x[1]): o.write(f"{k1}\t{ln}\t{100.0 * c / T:.4f}\t{t}\n")
if os.environ.get("B_SITES"):
    print("call sites by class:")
    for (k1, st), c in sorted(sites.items(), key=lambda x: (x[0][0], -x[1])):
        if k1 != "load (excluded)" and 100.0 * c / T >= 0.05: print(f"  {100.0 * c / T:6.2f}%  {k1} | {st}")
