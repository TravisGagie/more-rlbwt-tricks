## The `.lvl` by component

*Commit hashes in this README refer to the private development history of this code; those commits are not in this repository.*

| | 16, ▷ | 16, ◇ | 32, ▷ | 32, ◇ | 64, ▷ | 64, ◇ |
|---|---:|---:|---:|---:|---:|---:|
| B (text rows to parse rows) | 13.4 | 13.4 | 15.4 | 15.4 | 18.0 | 18.0 |
| the parse's BWT | 49.7 | 49.7 | 64.5 | 64.5 | 90.0 | 90.0 |
| the dictionary (hashes, id, rc id) | 105.2 | 105.2 | 116.7 | 116.7 | 133.6 | 133.6 |
| successors | 25.5 | 25.5 | 28.6 | 28.6 | 33.0 | 33.0 |
| continuation bits | 1.1 | 1.1 | 1.2 | 1.2 | 1.4 | 1.4 |
| pair starts (BS) | 1.9 | 2.3 | 3.0 | 3.1 | 4.8 | 4.3 |
| the BWT of the pairs | 12.7 | 16.0 | 22.3 | 27.0 | 39.5 | 47.2 |
| symbols: dstart | 2.2 | 2.3 | 2.5 | 2.7 | 3.0 | 3.2 |
| symbols: j | 1.5 | 2.0 | 1.8 | 2.5 | 2.6 | 3.4 |
| symbols: sides | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 | 0.0 |
| **parse** (B, parse's BWT, dictionary) | 168.3 | 168.3 | 196.6 | 196.6 | 241.6 | 241.6 |
| **pairs** (the rest) | 45.0 | 49.2 | 59.5 | 65.0 | 84.3 | 92.5 |

## Stored phrases and what goes with them

| | 16 | 32 | 64 |
|---|---:|---:|---:|
| whole phrases | 159.9 | 179.9 | 211.3 |
| without the first k (the right end) | 49.3 | 56.3 | 68.4 |
| without the last k (the left end) | 49.3 | 56.3 | 68.4 |
| one copy for both ends: without the first k, plus each range's k-mer once | 148.4 | 164.1 | 189.1 |
| **two short copies:** without the first k, and without the last k | 98.6 | 112.6 | 136.9 |
| — of which, phrases with X or $ (as the parse oracle stores them) | 1.6 | 1.8 | 2.1 |
| the ranges of identifiers (a pointer per phrase, a bit per phrase) | 25.5 | 28.6 | 34.4 |
| the parse's BWT by row (run heads: Elias–Fano starts, a symbol per run) | 39.2 | 50.3 | 68.8 |
| phrases / runs in the parse's BWT | 8,512,838 / 10,332,018 | 9,522,052 / 12,954,298 | 11,011,641 / 17,449,211 |

## Totals without the index of the text

Each: the parse's BWT + the dictionary + two short copies of the phrases + the ranges + the parse's BWT by row; ▷ and ◇ add their pairs. B, the move structure, the RLBWT and the 8-mer table go. (With one copy for both ends instead, add 49.7 / 51.6 / 52.2 MB.)

| | 16 | 32 | 64 |
|---|---:|---:|---:|
| **plain two-level, without the index of the text** | **318.2** | **372.6** | **463.7** |
| plain two-level, the paper's size | 1,068 | 1,183 | 1,362 |
| ratio | 0.30 | 0.31 | 0.34 |
| **▷, without the index of the text** | **363.2** | **432.1** | **548.1** |
| ▷, the paper's size | 1,070 | 1,186 | 1,365 |
| ratio | 0.34 | 0.36 | 0.40 |
| **◇, without the index of the text** | **367.4** | **437.6** | **556.2** |
| ◇, the paper's size | 1,073.9 | 1,191.3 | 1,373.2 |
| ratio | 0.34 | 0.37 | 0.41 |
| the plain move structure (paper) | 668 | 716 | 791 |
| the RLCSA of the text (paper) | 579 | 1,193 | 1,301 |
