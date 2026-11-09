// Record agent positions from a checkpoint for visualisation.
// Usage: replay CHECKPOINT STEPS EVERY > frames.json
// Output: {"L":..,"t0":..,"every":..,"frames":[[[id,x64,y64,dir,pen],...],...],"births":[..]}
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "geb/serial.hpp"
#include "geb/world.hpp"

using namespace oee::geb;

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
      std::printf("%s[%llu,%d,%d,%d,%d]", f2 ? "" : ",", (unsigned long long)o->id, o->at_x_64,
                  o->at_y_64, o->direction_deg, o->pen_colour);
      f2 = false;
    }
    std::printf("]");
  }
  std::printf("],\"births\":[");
  for (std::size_t i = 0; i < births.size(); ++i) std::printf("%s%ld", i ? "," : "", births[i]);
  std::printf("]}\n");
}
