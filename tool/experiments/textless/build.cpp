// Two-level search without the index of the text: builds PREFIX.tlx (src/textless.hpp) from PREFIX.lvl and the parse
// oracle's dictionary (PTXT, the distinct phrases in the .lvl's identifier order; only its dictionary is read).
//   bin/textless-build PREFIX PTXT [OUT.tl2]   (writes PREFIX.tlx; with OUT.tl2, also the minimal-width version there)
// Build, from tool/:
//   g++ -std=c++17 -O3 -march=native -DNDEBUG -Isrc -Ithird_party/libsais/include experiments/textless/build.cpp
//       build/libsais.o build/libsais64.o -o bin/textless-build -lz -pthread
#include "textless.hpp"
using namespace katka;

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "usage: textless-build PREFIX PTXT\n"); return 1; }
    const std::string prefix = argv[1];
    double t0 = now();
    setenv("KATKA_NO_FIRST_RUN", "1", 1);
    two_level L; L.load(prefix + ".lvl");
    packed_dict d; PackedInts o;
    u64 n, m; uint32_t K, SM, sigma;
    {
        FILE *f = open_or_die(argv[2], "rb"); rmagic(f, "KTPTXT02", argv[2]);
        rd(f, n); rd(f, m); rd(f, K); rd(f, SM); rd(f, sigma);
        d.load(f); o.load(f);
        fclose(f);
    }
    if (sigma != L.sigma || (int32_t)K != L.K || (int32_t)SM != L.SM || m != L.m) die("the .ptxt does not match the .lvl");
    textless_index T;
    T.build(L, std::move(d), std::move(o), K);
    T.save(prefix + ".tlx");
    if (argc > 3) {
        textless_index2 T2; T2.build(T); T2.save(argv[3]);
        fprintf(stderr, "%s: MB: phrases %.3f, ranges %.3f, by row %.3f\n", argv[3], T2.bytes_phrases() / 1e6, T2.bytes_ranges() / 1e6, T2.bytes_rows() / 1e6);
    }
    fprintf(stderr, "%s.tlx: sigma %lu, %lu ranges, %lu runs in the parse's BWT; MB: phrases %.3f (bases %.3f, offsets %.3f), ranges %.3f, by row %.3f; %.1f s\n",
            prefix.c_str(), T.sigma, T.R, T.hruns, T.bytes_phrases() / 1e6, T.dict.w.size() * 8 / 1e6, T.doff.bytes() / 1e6, T.bytes_ranges() / 1e6,
            T.bytes_rows() / 1e6, now() - t0);
    return 0;
}
