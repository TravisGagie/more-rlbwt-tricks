#!/usr/bin/env python3
# Latency hiding on the move structure, part 1, step 5: where in its 64-byte line each sampled L3-miss load falls
# (8 records of 8 bytes per line).  The LF step's destination row and the rows read while fast-forwarding share one
# load instruction per end (move_index::step is not inlined; see the disassembly in move1/README), so they are told
# apart by address: destination rows are spread evenly over the 8 slots, while a fast-forward read misses only when it
# crosses into the next line, at slot 0.  The excess at slot 0 over the other slots' mean bounds the fast-forward
# misses.   python3 p1_slots.py PERF.DATA
import sys, subprocess, collections
out = subprocess.run(["perf", "script", "-i", sys.argv[1], "-F", "ip,addr"], capture_output=True, text=True).stdout
h = collections.Counter(); t = 0
for l in out.splitlines():
    f = l.split()
    if len(f) < 2: continue
    h[(int(f[0], 16) % 64) // 8] += 1; t += 1
mean = sum(h[i] for i in range(1, 8)) / 7
print(f"samples {t}; share per slot: " + " ".join(f"{i}: {100 * h[i] / t:.2f}%" for i in range(8)))
print(f"excess at slot 0 (fast-forward reads crossing into a new line, at most): {100 * (h[0] - mean) / t:.2f}% of the samples")
