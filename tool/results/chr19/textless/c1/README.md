# Stage 2, part C1: the minimal-width text-free index (2026-11-23)

*Commit hashes in this README refer to the private development history of this code; those commits are not in this repository.*

**What is built:**
- **`c<N>tl.lvl`:** the plain `c<N>.lvl` without its pairs and without B (`bin/strip-pairs IN OUT --no-B`). It holds the
  parse's BWT and the dictionary.
- **`c<N>tl.tl2`** (`bin/textless-build PREFIX PTXT OUT.tl2`; `textless_index2` in `src/textless.hpp`):
  - two short copies of each phrase, without its first k characters and without its last k, at 2 bits per base, with
    one Elias–Fano array of offsets for both;
  - the 89–328 phrases with X or $, stored whole;
  - a bit per phrase marking where ranges start;
  - a pointer per phrase to its last k-mer's range;
  - the parse's BWT by row.
- **`find --twolevel --no-jumps --textless PREFIX`** uses `PREFIX.tl2` if it exists, else `PREFIX.tlx` (part B's
  whole-phrase version).

**Files as built (MB = 10^6 bytes), 16 / 32 / 64 individuals:**

| | 16 | 32 | 64 |
|---|---:|---:|---:|
| `c<N>tl.lvl` | 153.6 | 179.6 | 221.5 |
| `c<N>tl.tl2` | 159.7 | 189.9 | 248.9 |
| **total** | **313.3** | **369.5** | **470.4** |
| part B as built (`c<N>.lvl`'s parse and dictionary + `c<N>.tlx`) | 425.7 | 491.0 | 596.8 |
| part A's designed total | 318.2 | 372.6 | 463.7 |

**When α has no identifier** (B2 case 2), the conservative rule applies.
- In a window test, the search reports α's left boundary + 1, which is correct: α is complete in the window and not a
  phrase, so the pattern from its left boundary doesn't occur.
- In an exact search, or with no α inside the window, the search stops with an error. It never returns an unverified
  position.
- **Counts** (`check/q<N>.check.log`):
  - α's left boundary reported: 1,011 / 463 / 301;
  - α without an identifier in an exact search: 0 / 0 / 0;
  - stopped: 0 / 0 / 0.

**Verification** (`check/`, `experiments/textless/check.sh` with `SUF=v`, which uses `c<N>.lvl`, with B for the
comparison, plus `c<N>tl.tl2`):
- every search was repeated with two-level search on the index of the text: 0 differences in j and occurrences;
- the MEMs equal plain search's, and the occurrences equal its intervals' sizes.
- The window tests with a shorter skip are 6,201 / 3,864 / 2,950, a few more than part B's 6,074 / 3,803 / 2,915,
  because of α's left boundary.

**Speed:** in one quick round (pinned, without huge pages, one run each), the minimal-width version was 0.7–1.9% faster
than part B's, plain and with the table and lookahead. Both get timed in the final session.
