#!/usr/bin/env python3
# Table for experiments/fb_timing.sh (2026-11-10): FB medians from results/chr19/fb/logs/, BML medians from the
# committed runs of the same setup (results/chr19/unique/runs.tsv for blocks and unique,
# results/chr19/pshortcuts/runs.tsv for heavy09), and the index size as in Table 1: every file find loads
# (.rlbwt + .csa + .lvl, + the shortcut file).  From tool/:  experiments/fb_table.py > results/chr19/fb/table.md
import csv, os, re, statistics
D = os.path.expanduser('~/research/rlbwt-tricks/chr19')
O = 'results/chr19/fb'
IND = {'33': 16, '65': 32, '129': 64}
NAME = {'blocks': 'two-level, blocks (smallest successor)', 'unique': 'two-level, unitig jumps',
        'heavy09': 'two-level, heavy-path shortcuts (θ = 0.9)', 'nojumps': 'two-level, no block steps',
        'plain': 'plain backward search (RLCSA, both strands)',
        'heavy08': 'two-level, heavy-path shortcuts (θ = 0.8)', 'heavy095': 'two-level, heavy-path shortcuts (θ = 0.95)',
        'ref4': "two-level, shortcuts from CHM13's parse (g = 4)", 'ref16': "two-level, shortcuts from CHM13's parse (g = 16)",
        'ref64': "two-level, shortcuts from CHM13's parse (g = 64)"}
# plain BML: Table 1's runs, results/chr19/README.md (40.4, 50.7, 59.6 s)
PLAIN_BML = {'33': 40.4, '65': 50.7, '129': 59.6}

def bml(n, c):
    if c == 'plain':
        return PLAIN_BML[n]
    if c == 'nojumps':
        rows = [r for r in csv.DictReader(open('results/chr19/unique/runs.tsv'), delimiter='\t') if r['n'] == n and r['index'] == 'smallest (no block steps)']
        return float(rows[0]['med'])
    if c.startswith(('heavy', 'ref')):
        rows = [r for r in csv.DictReader(open('results/chr19/pshortcuts/runs.tsv'), delimiter='\t') if r['n'] == n and r['cfg'] == 'shortcuts ' + c]
    else:
        rows = [r for r in csv.DictReader(open('results/chr19/unique/runs.tsv'), delimiter='\t') if r['n'] == n and r['index'] == {'blocks': 'smallest', 'unique': 'unique'}[c]]
    return float(rows[0]['med'])

def mb(n, c):
    P = '%s/c%s' % (D, n)
    lvl = '%s/unique/c%su.lvl' % (D, n) if c == 'unique' else P + '.lvl'
    files = [P + '.rlbwt', P + '.csa'] + ([] if c == 'plain' else [lvl]) + (['%s/psc/c%s.%s.psc' % (D, n, c)] if c.startswith(('heavy', 'ref')) else [])
    return sum(os.path.getsize(f) for f in files) / 1e6

def bml_today(n, c):   # experiments/bml_retime.sh, run beside the FB runs (2026-11-11's configurations only)
    fs = ['%s/logs/q%s.%s.bml.%d.log' % (O, n, c, r) for r in (1, 2, 3)]
    if not all(os.path.exists(f) for f in fs): return ''
    return '%.2f' % statistics.median(float(re.search(r'wall ([\d.]+) s,', open(f).read()).group(1)) for f in fs)

def fb(n, c):
    ts, stats = [], None
    for r in (1, 2, 3):
        x = open('%s/logs/q%s.%s.fb.%d.log' % (O, n, c, r)).read()
        ts.append(float(re.search(r'wall ([\d.]+) s,', x).group(1)))
        if stats is None:
            m = re.search(r'searches=(\d+) char_steps=(\d+) parse_steps=(\d+) block_steps=(\d+)', x)
            stats = dict(zip(('searches', 'char', 'parse', 'block'), map(int, m.groups()))) if m else {k: '–' for k in ('searches', 'char', 'parse', 'block')}
            m = re.search(r'taken=(\d+) \(phrases crossed (\d+)\)', x)
            stats['taken'] = int(m.group(1)) if m else 0
            stats['mems'] = int(re.search(r'MEMs=(\d+)', x).group(1))
    return ts, stats

print('| method | individuals | BML s | BML s, rerun beside FB | FB s (median of 3) | FB runs | MB (files loaded) | FB searches | char steps | parse steps | block steps | shortcuts taken | MEMs (= BML) |')
print('|---|---|---:|---:|---:|---|---:|---:|---:|---:|---:|---:|---:|')
for c in ('plain', 'nojumps', 'blocks', 'unique', 'ref4', 'ref16', 'ref64', 'heavy08', 'heavy09', 'heavy095'):
    for n in ('33', '65', '129'):
        ts, s = fb(n, c)
        print('| %s | %d | %.2f | %s | %.2f | %s | %.1f | %s | %s | %s | %s | %s | %s |' % (
            NAME[c], IND[n], bml(n, c), bml_today(n, c), statistics.median(ts), ' '.join('%.2f' % t for t in ts), mb(n, c),
            *(s[k] if isinstance(s[k], str) else '{:,}'.format(s[k]) for k in ('searches', 'char', 'parse', 'block', 'taken', 'mems'))))
