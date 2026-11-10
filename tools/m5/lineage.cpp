// Lineage test for the M5 layout [parent | random buffer] (tape = 2 * genome length):
// child = buffer. Iterate G generations from each founder; report per generation the
// number of bytes the child shares with its parent (best alignment within +-8), and
// whether the lineage keeps copying (child copies at least 16 bytes).
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "bff/soup.hpp"

using namespace oee::bff;

int best_shift(const std::vector<unsigned char>& a, const std::vector<unsigned char>& b) {
  int best = 0;
  for (int sh = -8; sh <= 8; ++sh) {
    int m = 0;
    for (int k = 0; k < (int)a.size(); ++k) {
      const int j = k + sh;
      if (j >= 0 && j < (int)b.size()) m += a[k] == b[j];
    }
    best = std::max(best, m);
  }
  return best;
}

int main(int argc, char** argv) {
  std::vector<std::vector<unsigned char>> F;
  std::FILE* f = std::fopen(argv[1], "r");
  char line[65536];
  while (std::fgets(line, sizeof line, f)) {
    if (line[0] == '#') continue;
    std::vector<unsigned char> b;
    for (char* c = line; c[0] && c[1] && c[0] != '\n'; c += 2) b.push_back(std::strtoul(std::string(c, 2).c_str(), nullptr, 16));
    if (!b.empty()) F.push_back(b);
  }
  const int G = argc > 2 ? std::atoi(argv[2]) : 20, R = argc > 3 ? std::atoi(argv[3]) : 50;
  std::uint64_t s = 7;
  for (std::size_t i = 0; i < F.size(); ++i) {
    std::vector<double> share(G, 0);
    int alive_end = 0;
    for (int r = 0; r < R; ++r) {
      std::vector<unsigned char> p = F[i];
      bool alive = true;
      for (int g = 0; g < G; ++g) {
        std::vector<unsigned char> tape(p);
        for (std::size_t k = 0; k < p.size(); ++k) tape.push_back(splitmix64(s++) & 255);
        for (auto& x : tape) if ((splitmix64(s++) & 4095) == 0) x = splitmix64(s++) & 255;
        evaluate_n(tape.data(), tape.size(), kMaxSteps);
        std::vector<unsigned char> child(tape.begin() + p.size(), tape.end());
        const int m = best_shift(p, child);
        share[g] += m;
        if (m < 16) alive = false;
        p = child;
      }
      alive_end += alive;
    }
    std::printf("founder %zu: shared bytes per generation:", i);
    for (int g = 0; g < G; g += std::max(1, G / 8)) std::printf(" g%d=%.0f", g + 1, share[g] / R);
    std::printf("  | lineages still copying >=16 bytes at g%d: %d/%d\n", G, alive_end, R);
  }
}
