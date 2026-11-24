// Stage 2, part C0: PREFIX.lvl without its pairs (blocks), for the rows that never read them (--no-jumps, the parse
// shortcuts, STPD over the parse).  Loads the two-level index and saves it with has_blocks = 0.
//   bin/strip-pairs IN.lvl OUT.lvl [--no-B] [--keep-pairs]   (--no-B: also without B, for two-level search without the
//   index of the text; --keep-pairs: keep the pairs, for the encodings without the index of the text)
#include "twolevel.hpp"
using namespace katka;
int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: strip-pairs IN.lvl OUT.lvl\n"); return 1; }
    setenv("KATKA_NO_FIRST_RUN", "1", 1);
    two_level L; L.load(argv[1]);
    bool noB = false, keep = false;
    for (int a = 3; a < argc; ++a) { const std::string o = argv[a]; if (o == "--no-B") noB = true; else if (o == "--keep-pairs") keep = true; else die("unknown option " + o); }
    if (!keep) { L.has_blocks = 0; L.maxblock = 0; }
    if (noB) L.B = RLBits();
    L.save(argv[2]);
    return 0;
}
