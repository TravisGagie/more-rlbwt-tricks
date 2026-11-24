#!/usr/bin/env python3
# Part A's tables from bin/textless-sizes' output: python3 sizes_table.py DIR > tables.md (MB = 10^6 bytes)
import sys
D = sys.argv[1]; NS = [(33, 16), (65, 32), (129, 64)]
def rd(n, s):
    r = {l.split("\t")[0]: int(l.split("\t")[1]) for l in open(f"{D}/c{n}{s}.tsv")}
    r["two_short"] = 2 * r["copy_no_first_k_total"]; return r
mb = lambda b: f"{b / 1e6:,.1f}"
PAPER = {"plain": (1068, 1183, 1362), "u": (1070, 1186, 1365), "w": (1073.9, 1191.3, 1373.2)}
MOVE, RLCSA = (668, 716, 791), (579, 1193, 1301)
print("## The `.lvl` by component\n\n| | 16, ▷ | 16, ◇ | 32, ▷ | 32, ◇ | 64, ▷ | 64, ◇ |\n|---|---:|---:|---:|---:|---:|---:|")
K = [("lvl_B", "B (text rows to parse rows)"), ("lvl_pbwt", "the parse's BWT"), ("lvl_dict", "the dictionary (hashes, id, rc id)"),
     ("lvl_succ", "successors"), ("lvl_ccont", "continuation bits"), ("lvl_BS", "pair starts (BS)"), ("lvl_bbwt", "the BWT of the pairs"),
     ("lvl_dstart", "symbols: dstart"), ("lvl_jv", "symbols: j"), ("lvl_sidev", "symbols: sides")]
for k, nm in K: print(f"| {nm} | " + " | ".join(mb(rd(n, s)[k]) for n, _ in NS for s in ("u", "w")) + " |")
print("| **parse** (B, parse's BWT, dictionary) | " + " | ".join(mb(rd(n, s)["lvl_parse"]) for n, _ in NS for s in ("u", "w")) + " |")
print("| **pairs** (the rest) | " + " | ".join(mb(rd(n, s)["lvl_blocks"]) for n, _ in NS for s in ("u", "w")) + " |")
print("\n## Stored phrases and what goes with them\n\n| | 16 | 32 | 64 |\n|---|---:|---:|---:|")
for k, nm in [("copy_whole_total", "whole phrases"), ("copy_no_first_k_total", "without the first k (the right end)"),
              ("copy_no_last_k_total", "without the last k (the left end)"),
              ("copy_no_first_k_plus_range_kmers_total", "one copy for both ends: without the first k, plus each range's k-mer once"),
              ("two_short", "**two short copies:** without the first k, and without the last k"),
              ("other_chars", "— of which, phrases with X or $ (as the parse oracle stores them)"),
              ("ranges_total", "the ranges of identifiers (a pointer per phrase, a bit per phrase)"),
              ("pbwt_by_row", "the parse's BWT by row (run heads: Elias–Fano starts, a symbol per run)")]:
    print(f"| {nm} | " + " | ".join(mb(rd(n, "")[k]) for n, _ in NS) + " |")
print("| phrases / runs in the parse's BWT | " + " | ".join(f"{rd(n, '')['sigma']:,} / {rd(n, '')['pbwt_runs']:,}" for n, _ in NS) + " |")
print("\n## Totals without the index of the text\n\nEach: the parse's BWT + the dictionary + two short copies of the phrases + the ranges + the parse's BWT by row; "
      "▷ and ◇ add their pairs. B, the move structure, the RLBWT and the 8-mer table go. (With one copy for both ends instead, add "
      + " / ".join(mb(rd(n, "")["copy_no_first_k_plus_range_kmers_total"] - rd(n, "")["two_short"]) for n, _ in NS) + " MB.)\n\n| | 16 | 32 | 64 |\n|---|---:|---:|---:|")
def base(n, s):
    r = rd(n, s); return r["lvl_pbwt"] + r["lvl_dict"] + rd(n, "")["two_short"] + rd(n, "")["ranges_total"] + rd(n, "")["pbwt_by_row"]
for s, nm, key in (("", "plain two-level", "plain"), ("u", "▷", "u"), ("w", "◇", "w")):
    t = [base(n, s) + (rd(n, s)["lvl_blocks"] if s else 0) for n, _ in NS]
    print(f"| **{nm}, without the index of the text** | " + " | ".join(f"**{mb(x)}**" for x in t) + " |")
    print(f"| {nm}, the paper's size | " + " | ".join(f"{p:,}" for p in PAPER[key]) + " |")
    print(f"| ratio | " + " | ".join(f"{x / 1e6 / p:.2f}" for x, p in zip(t, PAPER[key])) + " |")
print("| the plain move structure (paper) | " + " | ".join(f"{x:,}" for x in MOVE) + " |")
print("| the RLCSA of the text (paper) | " + " | ".join(f"{x:,}" for x in RLCSA) + " |")
