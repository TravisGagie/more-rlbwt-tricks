#!/usr/bin/env python3
"""Writes the forward sequences of a katka text back out as FASTA, one file per sequence.

    fasta_from_S.py PREFIX OUTDIR LIST

PREFIX.S is S = G_1 X rc(G_1) X G_2 ...; PREFIX.tbl has one row per sequence (index, name, file, length, start).
Writes OUTDIR/s<NNN>.fa (header >s<i>, the sequence on one line) and LIST (as for katka prep: "-<TAB>path").
Used for the published STPD-index at 32 and 64 individuals, whose FASTA inputs are not on this machine;
checked at 16 against the original FASTAs (stpd-prep gives a byte-identical st.T).
"""
import mmap, os, sys
prefix, out, lst = sys.argv[1:4]
os.makedirs(out, exist_ok=True)
S = mmap.mmap(os.open(prefix + ".S", os.O_RDONLY), 0, prot=mmap.PROT_READ)
with open(lst, "w") as L:
    for row in open(prefix + ".tbl"):
        i, _, _, n, st = row.rstrip("\n").split("\t")[:5]
        i, n, st = int(i), int(n), int(st)
        f = os.path.join(out, "s%03d.fa" % (i + 1))
        with open(f, "wb") as o:
            o.write(b">s%d\n" % (i + 1)); o.write(S[st:st + n]); o.write(b"\n")
        L.write("-\t%s\n" % os.path.abspath(f))
