// evaluate_n(tape, 128) must equal evaluate(tape) on random and replicator tapes.
#include <cstdio>
#include <cstring>

#include "bff/soup.hpp"

using namespace oee::bff;

int main() {
  int bad = 0;
  for (std::uint64_t k = 0; k < 200000; ++k) {
    std::uint8_t a[kPair], b[kPair];
    for (int i = 0; i < kPair; ++i) {
      std::uint64_t r = splitmix64(k * kPair + i);
      // Bias towards command bytes so loops and copies are exercised.
      static const char cmds[] = "[]+-.,<>{}";
      a[i] = (r & 3) ? static_cast<std::uint8_t>(cmds[(r >> 8) % 10]) : static_cast<std::uint8_t>(r >> 16);
    }
    std::memcpy(b, a, kPair);
    const auto x = evaluate(a), y = evaluate_n(b, kPair, kMaxSteps);
    if (x != y || std::memcmp(a, b, kPair)) ++bad;
  }
  std::printf("mismatches: %d of 200000\n", bad);
  return bad != 0;
}
