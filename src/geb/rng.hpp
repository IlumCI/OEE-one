// Port of Geb v08b3 rnd_core (Numerical Recipes ran1 as adapted by D. Cliff).
// Bit-exact with the original; see docs/GEB_SPEC.md section 1.
#pragma once

#include <array>
#include <cstdint>

namespace oee::geb {

class Ran1 {
 public:
  // seed == 0 seeds from the clock (as the original); returns the seed used.
  int seed(int seed);
  // Uniform integer in [0, m-1]; 0 if m == 0.
  int random(int m);

 private:
  float next();
  double randval(double limit);

  int ix1_ = 0, ix2_ = 0, ix3_ = 0;
  std::array<float, 98> r_{};
  int iff_ = 0;
};

}  // namespace oee::geb
