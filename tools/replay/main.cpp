// Record agent positions from a checkpoint for visualisation.
// Usage: replay CHECKPOINT STEPS EVERY [grid|colonies] > out
// Default output is JSON (below). With "grid", output is raw bytes: per recorded cycle,
// L*L cell states in row-major order: 0 = empty, else 1 + (heading quadrant) + 4 * (pen + 1)
// (pen: -1 none, 1 reproduce, 2 kill attempted this cycle), i.e. states 1..16.
// Output: {"L":..,"t0":..,"every":..,"frames":[[[id,x64,y64,dir,pen,nodes,sprite],...],...],"births":[..]}
// sprite: 15-bit MinHash signature of the agent's production rules. A mutation that
// changes one rule changes only the bits whose minimum came from that rule, so
// related genotypes have similar signatures.
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include "geb/serial.hpp"
#include "geb/world.hpp"

using namespace oee::geb;

namespace {
std::uint64_t mix(std::uint64_t x) {
  x ^= x >> 33; x *= 0xff51afd7ed558ccdULL; x ^= x >> 33; x *= 0xc4ceb9fe1a85ec53ULL; x ^= x >> 33;
  return x;
}
int sprite(const Organism& o) {
  std::vector<std::uint64_t> hs;
  for (const Rule& r : o.rules()) {
    std::uint64_t h = 1469598103934665603ULL;
    for (int k = 0; k < r.gene_string.length(); ++k) h = (h ^ static_cast<std::uint8_t>(r.gene_string.at(k))) * 1099511628211ULL;
    hs.push_back(h);
  }
  int bits = 0;
  for (int j = 0; j < 15; ++j) {
    std::uint64_t best = ~0ULL;
    for (std::uint64_t h : hs) best = std::min(best, mix(h ^ (0x9e3779b97f4a7c15ULL * (j + 1))));
    if (!hs.empty() && (best >> 7 & 1)) bits |= 1 << j;
  }
  return bits;
}
}  // namespace

int main(int argc, char** argv) {
  if (argc < 4) return 2;
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
  const long steps = std::atol(argv[2]), every = std::atol(argv[3]);
  if (argc > 4 && std::string(argv[4]) == "grid") {
    std::vector<unsigned char> row(w->world_size());
    const long end = w->time() + steps;
    while (w->time() < end) {
      const long before = w->time();
      w->step();
      if (w->time() == before || w->time() % every) continue;
      for (int c = 0; c < w->world_size(); ++c) {
        const Organism* o = w->at(c);
        row[c] = o ? static_cast<unsigned char>(1 + ((o->direction_deg % 360) / 90) + 4 * (o->pen_colour + 1)) : 0;
      }
      std::fwrite(row.data(), 1, row.size(), stdout);
    }
    return 0;
  }
  if (argc > 4 && std::string(argv[4]) == "colonies") {
    // Raw bytes per recorded cycle: u16 n; n x (u8 cell x, u8 cell y, u8 colony colour 0-5,
    // u8 flags: bit0 warrior, bit1-2 pen + 1); u16 m; m x (u16 i, u16 j) live bonds, i < j,
    // indices into this cycle's agent list. Header: u16 L.
    const unsigned short L = static_cast<unsigned short>(w->world_length());
    std::fwrite(&L, 2, 1, stdout);
    const long end = w->time() + steps;
    std::vector<unsigned char> rec;
    std::vector<unsigned short> edges;
    while (w->time() < end) {
      const long before = w->time();
      w->step();
      if (w->time() == before || w->time() % every) continue;
      rec.clear(); edges.clear();
      std::vector<const Organism*> list;
      for (int c = 0; c < w->world_size(); ++c) if (const Organism* o = w->at(c)) list.push_back(o);
      std::unordered_map<std::uint64_t, int> idx;
      for (int i = 0; i < static_cast<int>(list.size()); ++i) idx[list[i]->id] = i;
      for (int i = 0; i < static_cast<int>(list.size()); ++i) {
        const Organism* o = list[i];
        rec.push_back(static_cast<unsigned char>(o->at_x));
        rec.push_back(static_cast<unsigned char>(o->at_y));
        rec.push_back(static_cast<unsigned char>(mix(o->colony) % 6));
        rec.push_back(static_cast<unsigned char>((o->warrior() ? 1 : 0) | ((o->pen_colour + 1) << 1)));
        for (std::uint64_t id : o->bonds) {
          auto it = idx.find(id);
          if (it == idx.end() || it->second <= i) continue;
          const Organism* q = list[it->second];
          const int dx = std::abs(q->at_x - o->at_x), dy = std::abs(q->at_y - o->at_y);
          if (std::max(std::min(dx, L - dx), std::min(dy, L - dy)) > World::kBondRange) continue;
          edges.push_back(static_cast<unsigned short>(i));
          edges.push_back(static_cast<unsigned short>(it->second));
        }
      }
      const unsigned short nn = static_cast<unsigned short>(list.size()), m = static_cast<unsigned short>(edges.size() / 2);
      std::fwrite(&nn, 2, 1, stdout);
      std::fwrite(rec.data(), 1, rec.size(), stdout);
      std::fwrite(&m, 2, 1, stdout);
      std::fwrite(edges.data(), 2, edges.size(), stdout);
    }
    return 0;
  }
  std::printf("{\"L\":%d,\"t0\":%ld,\"every\":%ld,\"frames\":[", w->world_length(), w->time(), every);
  const long t_end = w->time() + steps;
  bool first = true;
  long births0 = w->num_reproductions();
  std::vector<long> births;
  while (w->time() < t_end) {
    const long before = w->time();
    w->step();
    if (w->time() == before || w->time() % every) continue;
    births.push_back(w->num_reproductions() - births0);
    std::printf("%s[", first ? "" : ",");
    first = false;
    bool f2 = true;
    for (int c = 0; c < w->world_size(); ++c) {
      const Organism* o = w->at(c);
      if (!o) continue;
      std::printf("%s[%llu,%d,%d,%d,%d,%d,%d]", f2 ? "" : ",", (unsigned long long)o->id, o->at_x_64,
                  o->at_y_64, o->direction_deg, o->pen_colour, o->num_nodes(), sprite(*o));
      f2 = false;
    }
    std::printf("]");
  }
  std::printf("],\"births\":[");
  for (std::size_t i = 0; i < births.size(); ++i) std::printf("%s%ld", i ? "," : "", births[i]);
  std::printf("]}\n");
}
