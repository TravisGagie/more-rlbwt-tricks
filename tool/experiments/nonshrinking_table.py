#!/usr/bin/env python3
# Summary of experiments/nonshrinking.sh's output (results/chr19/nonshrinking/*.txt) as markdown.  From tool/:
#   experiments/nonshrinking_table.py > results/chr19/nonshrinking/summary.md
import re, os
O = 'results/chr19/nonshrinking'
SIZES = [s for s in (16, 32, 64) if os.path.exists('%s/parse.%d.heavy09.txt' % (O, s)) and 'MEMs' in open('%s/parse.%d.heavy09.txt' % (O, s)).read()]
def num(x): return int(x)
def section(txt, name):
    i = txt.index('== %s' % name); j = txt.find('\n== ', i + 1); return txt[i:j if j > 0 else None]
def runs_block(sec, which):
    k = sec.index(which); return sec[k:]
def eps_lines(sec):
    out = []
    for m in re.finditer(r'eps = +(\d+)%: \(nearly\) non-shrinking (\d+) \(([\d.]+)% of steps\), runs (\d+), total run length (\d+) \(mean ([\d.]+)\); lower bound total - ns \+ runs = (\d+), best-possible speedup in steps ([\d.]+)\n +run lengths: (.*)', sec):
        out.append(m.groups())
    return out
def hist(h):
    d = dict(re.findall(r'([\d>=-]+): (\d+)', h))
    keys = list(d)
    top = sum(int(d[k]) for k in keys if k in ('1024-2047', '>=2048'))
    return ', '.join('%s: %s' % (k, '{:,}'.format(int(d[k]))) for k in keys if k not in ('1024-2047', '>=2048')) + ', ≥1024: ' + '{:,}'.format(top)
print('# How many backward steps could skipping save? (chr19, BML, L = 1000; counting only)\n')
print('Made by `experiments/nonshrinking_table.py` from `rs.<size>.txt` and `parse.<size>.<cfg>.txt` (`experiments/nonshrinking.sh`).\n')
for s in SIZES:
    t = open('%s/rs.%d.txt' % (O, s)).read()
    print('## %d individuals: the RLFM-index of the text and its reverse (Reportable Shortcuts\' RLBWTs)\n' % s)
    print(re.search(r'plain BML: \d+ MEMs', t).group(0) + '.\n')
    for g, label in (('backward searches', 'backward searches (window + MEM start + next start, text index)'), ('forward searches', 'forward searches (reversed index)'), ('all searches', 'all searches')):
        sec = section(t, g)
        h = re.search(r': (\d+) searches, (\d+) backward steps \((\d+) failed\); steps with \|I\(X\)\| = 1: (\d+) \(([\d.]+)%', sec).groups()
        print('**%s:** %s searches, %s backward steps (%s failed), %s from |I(X)| = 1 (%s%%).\n' % (label, *('{:,}'.format(int(x)) for x in h[:4]), h[4]))
        print('| eps | treatment of \\|I(X)\\| = 1 | (nearly) non-shrinking | % | runs | mean run | lower bound (total − ns + runs) | best speedup | run lengths |')
        print('|---|---|---:|---:|---:|---:|---:|---:|---|')
        for which, tl in (('excluding', 'excluded (breaks runs)'), ('counting successful', 'counted as non-shrinking')):
            for e in eps_lines(runs_block(sec, which).split('counting successful')[0] if which == 'excluding' else runs_block(sec, which)):
                print('| %s%% | %s | %s | %s | %s | %s | %s | %s | %s |' % (e[0], tl, '{:,}'.format(int(e[1])), e[2], '{:,}'.format(int(e[3])), e[5], '{:,}'.format(int(e[6])), e[7], hist(e[8])))
        print()
    print('**Per kind, eps = 0, |I(X)| = 1 excluded:**\n')
    print('| search | searches | steps | from \\|I(X)\\| = 1 | non-shrinking | runs | lower bound | best speedup |')
    print('|---|---:|---:|---:|---:|---:|---:|---:|')
    for k in ('window', 'mem', 'next', 'forward'):
        sec = section(t, k + ':')
        h = re.search(r': (\d+) searches, (\d+) backward steps .*?\|I\(X\)\| = 1: (\d+)', sec).groups()
        e = eps_lines(sec)[0]
        print('| %s | %s | %s | %s | %s | %s | %s | %s |' % (k, *('{:,}'.format(int(x)) for x in h), '{:,}'.format(int(e[1])), '{:,}'.format(int(e[3])), '{:,}'.format(int(e[6])), e[7]))
    print('\n**With almost-funnels (ℓ = 25), same BML, MEMs = plain:**\n')
    print('| almost-funnels | searches | backward steps | jumps | operations | characters crossed by jumps | plain steps / operations |')
    print('|---|---|---:|---:|---:|---:|---:|')
    for name, lab in (('tau0', 'fixed-length (τ = 0)'), ('tau6', 'variable-length (τ = 6)')):
        sec = t[t.index('== with shortcuts %s' % name):]
        sec = sec[:sec.find('\n\n') if '\n\n' in sec else None]
        assert ': 0 patterns with MEMs differing' in sec
        for m in re.finditer(r'  ([a-z ()+,]+?): (\d+) searches, (\d+) backward steps \+ (\d+) jumps = (\d+) operations; jumps crossed (\d+) characters .*?ratio ([\d.]+)', sec):
            kind = m.group(1).split(' (')[0]
            print('| %s | %s | %s | %s | %s | %s | %s |' % (lab, kind, *('{:,}'.format(int(x)) for x in m.groups()[2:6]), m.group(7)))
    print()
    print('## %d individuals: the two-level index, parse level (katka, `--csa --twolevel`)\n' % s)
    nj = open('%s/parse.%d.nojumps.txt' % (O, s)).read()
    h = re.search(r'nonshrinking parse steps: (\d+) parse steps \((\d+) failed\); from an interval of size 1: (\d+)', nj).groups()
    print('No block steps: %s parse steps (%s failed), %s from a parse interval of size 1 (excluded below; they break runs).\n' % tuple('{:,}'.format(int(x)) for x in h))
    print('| eps | non-shrinking | % | runs | mean run | lower bound | best speedup | run lengths |')
    print('|---|---:|---:|---:|---:|---:|---:|---|')
    for m in re.finditer(r'eps = (\d+)%: \(nearly\) non-shrinking (\d+) \(([\d.]+)%\), runs (\d+), total run length (\d+) \(mean ([\d.]+)\); lower bound steps - ns \+ runs = (\d+), best-possible speedup in parse steps ([\d.]+)\n  run lengths: (.*)', nj):
        g = m.groups()
        print('| %s%% | %s | %s | %s | %s | %s | %s | %s |' % (g[0], '{:,}'.format(int(g[1])), g[2], '{:,}'.format(int(g[3])), g[5], '{:,}'.format(int(g[6])), g[7], hist(g[8])))
    base = int(h[0])
    print('\n| configuration | parse steps | block steps | shortcuts taken (phrases crossed) | operations | no-jump parse steps / operations |')
    print('|---|---:|---:|---:|---:|---:|')
    for c, lab in (('nojumps', 'no block steps'), ('blocks', 'smallest-successor blocks'), ('heavy09', 'heavy-path almost-funnels, θ = 0.9')):
        x = open('%s/parse.%d.%s.txt' % (O, s, c)).read()
        p, b = map(int, re.search(r'parse_steps=(\d+) block_steps=(\d+)', x).groups())
        m = re.search(r'taken=(\d+) \(phrases crossed (\d+)\)', x)
        tk, cr = (int(m.group(1)), int(m.group(2))) if m else (0, 0)
        ops = p + b + tk
        print('| %s | %s | %s | %s | %s | %.3f |' % (lab, '{:,}'.format(p), '{:,}'.format(b), '{:,} ({:,})'.format(tk, cr) if tk else '0', '{:,}'.format(ops), base / ops))
    print()
