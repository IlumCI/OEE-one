// Heredity check for the M5 layout [p1 | buffer | p2]: fraction of births in which the
// buffer ends up an exact copy of p1 (or of p2), for founder genomes given as hex lines.
// Usage: m5_fidelity FOUNDERS.txt [TRIALS]
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "bff/soup.hpp"

using namespace oee::bff;

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
  const int trials = argc > 2 ? std::atoi(argv[2]) : 1000;
  std::uint64_t s = 1;
  auto rnd = [&]() { return splitmix64(s++); };
  for (std::size_t i = 0; i < F.size(); ++i)
    for (int pos = 0; pos < 2; ++pos) {
      int copy1 = 0, copy2 = 0, close1 = 0;
      double same_sum = 0, best_shift_sum = 0;
      for (int t = 0; t < trials; ++t) {
        std::vector<unsigned char> other(64);
        for (auto& x : other) x = rnd() & 255;            // random partner
        const auto& a = pos == 0 ? F[i] : other;
        const auto& b = pos == 0 ? other : F[i];
        std::vector<unsigned char> tape(a);
        for (std::size_t k = 0; k < a.size(); ++k) tape.push_back(rnd() & 255);
        if (std::getenv("WITH_P2")) tape.insert(tape.end(), b.begin(), b.end());
        const int n = tape.size();
        evaluate_n(tape.data(), n, std::size_t(kMaxSteps) * std::max(1, n / kPair));
        const unsigned char* child = tape.data() + a.size();
        int same = 0;
        for (std::size_t k = 0; k < a.size(); ++k) same += child[k] == F[i][k];
        copy1 += std::memcmp(child, a.data(), a.size()) == 0;
        copy2 += std::memcmp(child, b.data(), b.size()) == 0;
        close1 += same >= 60;
        same_sum += same;
        int best = 0;
        for (int sh = -8; sh <= 8; ++sh) {
          int m = 0;
          for (int k = 0; k < (int)a.size(); ++k) {
            const int j = k + sh;
            if (j >= 0 && j < (int)a.size()) m += child[j] == F[i][k];
          }
          best = std::max(best, m);
        }
        best_shift_sum += best;
      }
      std::printf("founder %zu as parent %d: exact %.1f%%  >=60/64 %.1f%%  mean bytes matching %.1f/64  best-shift %.1f/64\n", i,
                  pos + 1, 100.0 * copy1 / trials, 100.0 * close1 / trials, same_sum / trials, best_shift_sum / trials);
    }
}
