// H1 measure for M5: for every agent in a checkpoint, the number of genome bytes its own
// code passes to a child (best alignment within +-8), averaged over R births with random
// buffers, under the M5 v1 layout [genome | random buffer]. Also the fraction of agents
// that pass on at least 16 bytes ("copiers").
// Usage: m5_heritability CHECKPOINT [R]
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "bff/soup.hpp"
#include "geb/serial.hpp"
#include "geb/world.hpp"

using namespace oee::geb;

int main(int argc, char** argv) {
  if (argc < 2) return 2;
  std::FILE* f = std::fopen(argv[1], "rb");
  if (!f) return 2;
  std::vector<unsigned char> b;
  unsigned char buf[1 << 16];
  std::size_t n;
  while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) b.insert(b.end(), buf, buf + n);
  std::fclose(f);
  b.resize(b.size() - 8);
  Reader r(b.data(), b.size());
  r.get<std::uint64_t>();
  r.get_str();
  auto w = World::load(r);
  const int R = argc > 2 ? std::atoi(argv[2]) : 8;
  std::uint64_t s = 99;
  double total = 0;
  int agents = 0, copiers = 0;
  std::vector<int> hist(65, 0);
  for (int c = 0; c < w->world_size(); ++c) {
    const Organism* o = w->at(c);
    if (!o) continue;
    const Genotype& g = o->genotype();
    std::vector<unsigned char> p((g.size() + 7) / 8, 0);
    for (std::size_t i = 0; i < g.size(); ++i)
      if (g[i]) p[i / 8] |= 0x80 >> (i % 8);
    double sum = 0;
    for (int t = 0; t < R; ++t) {
      std::vector<unsigned char> tape(p);
      for (std::size_t k = 0; k < p.size(); ++k) tape.push_back(oee::bff::splitmix64(s++) & 255);
      oee::bff::evaluate_n(tape.data(), tape.size(),
                           std::size_t(oee::bff::kMaxSteps) * std::max<std::size_t>(1, tape.size() / oee::bff::kPair));
      int best = 0;
      for (int sh = -8; sh <= 8; ++sh) {
        int m = 0;
        for (int k = 0; k < (int)p.size(); ++k) {
          const int j = k + sh;
          if (j >= 0 && j < (int)p.size()) m += p[k] == tape[p.size() + j];
        }
        best = std::max(best, m);
      }
      sum += best;
    }
    const double mean = sum / R;
    total += mean;
    ++agents;
    copiers += mean >= 16;
    ++hist[std::min(64, (int)(mean + 0.5))];
  }
  std::printf("{\"time\": %ld, \"agents\": %d, \"mean_inherited_bytes\": %.2f, \"copier_fraction\": %.3f}\n", w->time(),
              agents, agents ? total / agents : 0.0, agents ? double(copiers) / agents : 0.0);
}
