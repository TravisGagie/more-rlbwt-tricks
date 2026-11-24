# The phrases that start with the same syncmer

*Commit hashes in this README refer to the private development history of this code; those commits are not in this repository.*

The phrases that start with the same k-mer have consecutive identifiers, since the identifiers follow the phrases'
lexicographic order and the parse is prefix-free. `c<N>.tsv` is the output of `bin/syncmer-ranges c<N>.ptxt`
(`experiments/diamond/syncmer_ranges.cpp`, on the parse oracle's dictionary) with 16 / 32 / 64 individuals: the check
that the identifiers are in order and that no first k-mer is split, the number of ranges and their sizes, and the sizes
of the structures the right end of two-level search without the index of the text uses. The number of ranges is
`experiments/textless/sizes.cpp`'s third input (part A). The rows weighted by search come from a counting build of the
search (`KATKA_DIACOUNT`) on the ▷ index.
