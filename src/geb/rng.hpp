// Port of Geb v08b3 rnd_core (Numerical Recipes ran1 as adapted by D. Cliff).
// Bit-exact with the original; see docs/GEB_SPEC.md section 1.
#pragma once

#include <array>
#include <cstdint>

#include "geb/serial.hpp"

namespace oee::geb {

class Ran1 {
 public:
  // seed == 0 seeds from the clock (as the original); returns the seed used.
  int seed(int seed);
  // Uniform integer in [0, m-1]; 0 if m == 0.
  int random(int m);

  void save(Writer& w) const {
    w.put(ix1_); w.put(ix2_); w.put(ix3_); w.put(r_); w.put(iff_);
  }
  void load(Reader& r) {
    ix1_ = r.get<int>(); ix2_ = r.get<int>(); ix3_ = r.get<int>();
    r_ = r.get<std::array<float, 98>>(); iff_ = r.get<int>();
  }

 private:
  float next();
  double randval(double limit);

  int ix1_ = 0, ix2_ = 0, ix3_ = 0;
  std::array<float, 98> r_{};
  int iff_ = 0;
};

}  // namespace oee::geb
