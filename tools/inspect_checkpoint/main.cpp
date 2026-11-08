// Print the agents in a checkpoint: position, heading, age, network size and each
// action's output (sum over matching network outputs, or "noise" if none match).
// Usage: inspect_checkpoint CHECKPOINT
#include <cstdio>
#include <cstring>
#include <vector>

#include "geb/serial.hpp"
#include "geb/world.hpp"

using namespace oee::geb;

namespace {
long action_sum(const Organism& o, const char* bits, bool& matched) {
  const Character pre(bits);
  long sum = -1;
  for (const auto& out : o.output_cache())
    if (pre.span().is_start_of(out.node->character)) {
      if (sum == -1) sum = 0;
      sum += out.io_value;
      if (sum > kExcitatoryScale) {
        sum = kExcitatoryScale;
        break;
      }
    }
  matched = sum != -1;
  return sum;
}
}  // namespace

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
  std::printf("time %ld agents %d world %d\n", w->time(), w->num_individuals(), w->world_length());
  const char* names[5] = {"repro", "kill", "turnL", "turnR", "fwd"};
  const char* pre[5] = {"01", "100", "101", "110", "111"};
  int ahead_occupied = 0, total = 0;
  for (int c = 0; c < w->world_size(); ++c) {
    const Organism* o = w->at(c);
    if (!o) continue;
    ++total;
    // Cell ahead (same stepping rule as forward1).
    int x64 = o->at_x_64, y64 = o->at_y_64, fx = x64 >> 6, fy = y64 >> 6, x = fx, y = fy;
    const int L64 = w->world_length() << 6;
    while (x == fx && y == fy) {
      x64 = (x64 + w->sindeg64(o->direction_deg) + L64) % L64;
      y64 = (y64 - w->cosdeg64(o->direction_deg) + L64) % L64;
      x = x64 >> 6;
      y = y64 >> 6;
    }
    const bool occ = w->at(x + y * w->world_length()) != nullptr;
    ahead_occupied += occ;
    std::printf("cell %3d (%2d,%2d) dir %3d age %8d nodes %2d pen %2d ahead_%s |", c, o->at_x, o->at_y,
                o->direction_deg, o->age, o->num_nodes(), o->pen_colour, occ ? "OCC" : "empty");
    for (int k = 0; k < 5; ++k) {
      bool m;
      const long s = action_sum(*o, pre[k], m);
      if (m)
        std::printf(" %s=%ld", names[k], s);
      else
        std::printf(" %s=noise", names[k]);
    }
    std::printf("\n");
  }
  std::printf("agents facing an occupied cell: %d of %d\n", ahead_occupied, total);
}
