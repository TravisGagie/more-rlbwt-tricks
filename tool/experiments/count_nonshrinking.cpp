// How many backward steps could skipping save?  (2026-11-11; counting only, untimed.)
//
// BML (L = min_length) exactly as Reportable Shortcuts' mems_bml (src/mems.cpp), on its RLBWTs of the text and
// of the reversed collection, with every plain backward step X -> cX recorded as (|I(X)|, |I(cX)|).  The searches
// are counted by kind:
//   window   BML's backward search of P[x .. x + L) (text index);
//   mem      the backward search for the MEM ending at e (text index);
//   next     the backward search from e + 1 for the next start (text index);
//   forward  the forward search P[x ..) (backward search in the reversed index).
// For each kind: the steps; the non-shrinking ones, |I(cX)| = |I(X)| (and the "nearly" non-shrinking ones,
// |I(cX)| >= (1 - eps)|I(X)|, eps = 1%, 5%, 10%), excluding and counted separately those with |I(X)| = 1; the
// maximal runs of consecutive such steps within one search, with a histogram of their lengths; and the lower
// bound total - nonshrinking + runs on the steps with perfect skipping (every run replaced by one jump).
// Steps with |I(X)| = 1 break the runs in the main counts (they are reported separately, and also in a second
// set of counts where they count as non-shrinking).
// Then the same BML with each set of shortcuts given (as query_mems runs it), with its backward steps, jumps and
// characters crossed per kind, and a check that every run gives the same MEMs as plain search.
//
//   count_nonshrinking INDEX_DIR REVERSED_DIR PATTERNS L [NAME=SHORTCUT_FILE_FWD,SHORTCUT_FILE_REV ...]
#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "mems.hpp"
#include "rlbwt.hpp"
#include "search.hpp"
#include "shortcuts.hpp"

using namespace rs;

namespace {

constexpr int KINDS = 4;
const char *KIND_NAME[KINDS] = {"window", "mem", "next", "forward"};
constexpr int NEPS = 4;
const double EPS[NEPS] = {0.0, 0.01, 0.05, 0.10};
constexpr int NBUCKETS = 12;   // 1, 2-3, 4-7, ..., 512-1023, >= 1024

int bucket(int64_t len) {
  int b = 0;
  while (b < NBUCKETS - 1 && (int64_t(2) << b) <= len) ++b;
  return b;
}

struct RunCounts {          // for one eps and one treatment of |I(X)| = 1
  int64_t ns = 0;           // steps counted as (nearly) non-shrinking
  int64_t runs = 0, run_len = 0;
  std::array<int64_t, NBUCKETS> hist{};
  int64_t cur = 0;          // the current run, within one search
  void step(bool counted) {
    if (counted) { ++ns; ++cur; return; }
    end();
  }
  void end() {
    if (cur) { ++runs; run_len += cur; ++hist[bucket(cur)]; cur = 0; }
  }
};

struct KindCounts {
  int64_t searches = 0, steps = 0, failed = 0, size1 = 0, size1_failed = 0;
  RunCounts excl[NEPS];     // |I(X)| = 1 breaks runs and is not counted
  RunCounts incl[NEPS];     // |I(X)| = 1 (a successful step) counts as non-shrinking
};

KindCounts K[KINDS];

// plain backward search for P[begin .. end), as search_range with sc = null, recording every step
SearchResult counted_search(const RLBWT &rlbwt, const std::string &P, int64_t begin, int64_t end, int kind) {
  KindCounts &k = K[kind];
  ++k.searches;
  SearchResult r;
  r.pattern_length = end - begin;
  int64_t lo = 0, hi = rlbwt.size() - 1;
  int64_t j = end;
  while (j > begin) {
    int64_t new_lo = lo, new_hi = hi;
    rlbwt.backward_step(static_cast<uint8_t>(P[static_cast<size_t>(j - 1)]), new_lo, new_hi);
    ++r.backward_steps;
    const int64_t s0 = hi - lo + 1, s1 = new_hi >= new_lo ? new_hi - new_lo + 1 : 0;
    ++k.steps;
    if (s1 == 0) ++k.failed;
    if (s0 == 1) { ++k.size1; if (s1 == 0) ++k.size1_failed; }
    for (int e = 0; e < NEPS; ++e) {
      const bool nearly = s1 > 0 && (double)s1 >= (1.0 - EPS[e]) * (double)s0;
      k.excl[e].step(nearly && s0 > 1);
      k.incl[e].step(nearly);
    }
    if (new_hi < new_lo) break;
    lo = new_lo; hi = new_hi; ++r.matched; --j;
  }
  for (int e = 0; e < NEPS; ++e) { k.excl[e].end(); k.incl[e].end(); }
  r.lo = lo; r.hi = hi;
  return r;
}

// the same BML, with or without shortcuts; kind-aware counters for the shortcut runs
struct SCCounts { int64_t searches = 0, steps = 0, jumps = 0, crossed = 0; };

struct Finder {
  const RLBWT &T, &Rv;
  const Shortcuts *sT, *sR;   // null: plain, counted step by step
  const std::string &P, Q;
  const int64_t m;
  SCCounts *sc;               // with shortcuts: per kind
  Finder(const RLBWT &t, const RLBWT &rv, const Shortcuts *st, const Shortcuts *sr, const std::string &p, SCCounts *c)
      : T(t), Rv(rv), sT(st), sR(sr), P(p), Q(p.rbegin(), p.rend()), m((int64_t)p.size()), sc(c) {}
  SearchResult search(bool reversed, int64_t b, int64_t e, int kind) {
    const RLBWT &I = reversed ? Rv : T;
    const std::string &X = reversed ? Q : P;
    if (!sc) return counted_search(I, X, b, e, kind);
    SearchResult r = search_range(I, reversed ? sR : sT, X, b, e);
    SCCounts &c = sc[kind];
    ++c.searches; c.steps += r.backward_steps; c.jumps += r.shortcuts; c.crossed += r.chars_crossed;
    return r;
  }
  int64_t forward(int64_t x) { return x + search(true, 0, m - x, 3).matched; }
  int64_t report_and_advance(int64_t x, int64_t e, int64_t L, std::vector<Mem> &out) {
    SearchResult r = search(false, 0, e, 1);
    const int64_t s = e - r.matched;
    if (e - s >= L) out.push_back({s, e, r.lo, r.hi});
    if (e == m) return m;
    const int64_t s1 = e + 1 - search(false, 0, e + 1, 2).matched;
    return std::max(s1, x + 1);
  }
  std::vector<Mem> bml(int64_t L) {
    std::vector<Mem> out;
    for (int64_t x = 0; x + L <= m;) {
      const int64_t we = x + L;
      const SearchResult r = search(false, x, we, 0);
      if (r.matched < L) { x = we - r.matched; continue; }
      x = report_and_advance(x, forward(x), L, out);
    }
    return out;
  }
};

void print_runs(const char *title, const KindCounts &k, const RunCounts (&rc)[NEPS], int64_t base) {
  printf("    %s\n", title);
  for (int e = 0; e < NEPS; ++e) {
    const RunCounts &c = rc[e];
    const int64_t lb = k.steps - c.ns + c.runs;
    printf("      eps = %4.0f%%: (nearly) non-shrinking %lld (%.2f%% of steps), runs %lld, total run length %lld (mean %.2f); "
           "lower bound total - ns + runs = %lld, best-possible speedup in steps %.3f\n",
           EPS[e] * 100, (long long)c.ns, 100.0 * c.ns / std::max<int64_t>(1, k.steps), (long long)c.runs, (long long)c.run_len,
           c.runs ? (double)c.run_len / c.runs : 0.0, (long long)lb, (double)base / std::max<int64_t>(1, lb));
    printf("        run lengths:");
    for (int b = 0; b < NBUCKETS; ++b) {
      if (b == NBUCKETS - 1) printf(" >=%lld: %lld", (long long)(int64_t(1) << b), (long long)c.hist[b]);
      else if (b == 0) printf(" 1: %lld,", (long long)c.hist[b]);
      else printf(" %lld-%lld: %lld,", (long long)(int64_t(1) << b), (long long)((int64_t(2) << b) - 1), (long long)c.hist[b]);
    }
    printf("\n");
  }
}

}  // namespace

int main(int argc, char **argv) {
  if (argc < 5) { fprintf(stderr, "usage: count_nonshrinking INDEX_DIR REVERSED_DIR PATTERNS L [NAME=FWD,REV ...]\n"); return 1; }
  const RLBWT T = RLBWT::load(std::string(argv[1]) + "/rlbwt.bin");
  const RLBWT Rv = RLBWT::load(std::string(argv[2]) + "/rlbwt.bin");
  const int64_t L = std::stoll(argv[4]);
  std::vector<std::string> patterns;
  { std::ifstream in(argv[3]); for (std::string p; std::getline(in, p);) { if (!p.empty() && p.back() == '\r') p.pop_back(); if (!p.empty()) patterns.push_back(p); } }
  struct SC { std::string name; Shortcuts f, r; };
  std::vector<SC> scs;
  for (int a = 5; a < argc; ++a) {
    std::string s = argv[a]; size_t eq = s.find('='), cm = s.find(',');
    if (eq == std::string::npos || cm == std::string::npos) throw std::runtime_error("shortcuts: NAME=FWD,REV");
    scs.push_back({s.substr(0, eq), Shortcuts::load(s.substr(eq + 1, cm - eq - 1)), Shortcuts::load(s.substr(cm + 1))});
  }
  printf("index %s, reversed %s, patterns %s (%zu), L = %lld\n", argv[1], argv[2], argv[3], patterns.size(), (long long)L);

  std::vector<std::vector<Mem>> plain_mems;
  int64_t nmems = 0;
  for (const std::string &P : patterns) { Finder f(T, Rv, nullptr, nullptr, P, nullptr); plain_mems.push_back(f.bml(L)); nmems += plain_mems.back().size(); }
  printf("plain BML: %lld MEMs\n", (long long)nmems);

  int64_t all_steps = 0;
  for (int k = 0; k < KINDS; ++k) all_steps += K[k].steps;
  const int groups[3][4] = {{0, 1, 2, -1}, {3, -1, -1, -1}, {0, 1, 2, 3}};
  const char *gname[3] = {"backward searches (window + mem + next, text index)", "forward searches (reversed index)", "all searches"};
  auto sum = [&](const int *g) {
    KindCounts s;
    for (int i = 0; i < 4 && g[i] >= 0; ++i) {
      const KindCounts &k = K[g[i]];
      s.searches += k.searches; s.steps += k.steps; s.failed += k.failed; s.size1 += k.size1; s.size1_failed += k.size1_failed;
      for (int e = 0; e < NEPS; ++e) for (int x = 0; x < 2; ++x) {
        RunCounts &d = x ? s.incl[e] : s.excl[e]; const RunCounts &o = x ? k.incl[e] : k.excl[e];
        d.ns += o.ns; d.runs += o.runs; d.run_len += o.run_len;
        for (int b = 0; b < NBUCKETS; ++b) d.hist[b] += o.hist[b];
      }
    }
    return s;
  };
  auto report = [&](const char *name, const KindCounts &k) {
    printf("\n== %s: %lld searches, %lld backward steps (%lld failed); steps with |I(X)| = 1: %lld (%.2f%%; %lld of them failed)\n",
           name, (long long)k.searches, (long long)k.steps, (long long)k.failed, (long long)k.size1, 100.0 * k.size1 / std::max<int64_t>(1, k.steps),
           (long long)k.size1_failed);
    print_runs("excluding steps with |I(X)| = 1 (they break runs):", k, k.excl, k.steps);
    print_runs("counting successful steps with |I(X)| = 1 as non-shrinking:", k, k.incl, k.steps);
  };
  for (int g = 0; g < 3; ++g) report(gname[g], sum(groups[g]));
  for (int k = 0; k < KINDS; ++k) report(KIND_NAME[k], K[k]);

  for (const SC &s : scs) {
    SCCounts c[KINDS];
    int64_t mism = 0;
    for (size_t q = 0; q < patterns.size(); ++q) {
      Finder f(T, Rv, &s.f, &s.r, patterns[q], c);
      if (f.bml(L) != plain_mems[q]) ++mism;
    }
    printf("\n== with shortcuts %s: %lld patterns with MEMs differing from plain search\n", s.name.c_str(), (long long)mism);
    SCCounts g[3];
    for (int gi = 0; gi < 3; ++gi)
      for (int i = 0; i < 4 && groups[gi][i] >= 0; ++i) { const SCCounts &o = c[groups[gi][i]]; g[gi].searches += o.searches; g[gi].steps += o.steps; g[gi].jumps += o.jumps; g[gi].crossed += o.crossed; }
    auto line = [&](const char *name, const SCCounts &x, int64_t plain_steps) {
      printf("  %s: %lld searches, %lld backward steps + %lld jumps = %lld operations; jumps crossed %lld characters "
             "(replacing that many character steps); plain search's steps %lld, ratio %.3f\n",
             name, (long long)x.searches, (long long)x.steps, (long long)x.jumps, (long long)(x.steps + x.jumps), (long long)x.crossed,
             (long long)plain_steps, (double)plain_steps / std::max<int64_t>(1, x.steps + x.jumps));
    };
    for (int gi = 0; gi < 3; ++gi) line(gname[gi], g[gi], sum(groups[gi]).steps);
    for (int k = 0; k < KINDS; ++k) line(KIND_NAME[k], c[k], K[k].steps);
  }
  (void)all_steps;
  return 0;
}
