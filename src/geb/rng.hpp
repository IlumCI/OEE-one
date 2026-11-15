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

// Counter-based stream: value i of stream (a,b,d) is a hash of the key and i, so any agent's
// draws are reproducible on their own (needed for order-equivalent parallel updates).
struct CRng{
std::uint64_t k=0,c=0;
static std::uint64_t mix(std::uint64_t z){z+=0x9e3779b97f4a7c15ULL;z=(z^(z>>30))*0xbf58476d1ce4e5b9ULL;z=(z^(z>>27))*0x94d049bb133111ebULL;return z^(z>>31);}
void key(std::uint64_t a,std::uint64_t b,std::uint64_t d){k=mix(mix(mix(a)^b)^d);c=0;}
int random(int m){if(m<=0)return 0;return int(double(mix(k+(c++))>>11)*0x1.0p-53*m);}
};

// Geb's ran1 (legacy, bit-exact) or a counter stream, chosen per world.
struct Rng{
Ran1*r1=nullptr;CRng c;
int random(int m){return r1?r1->random(m):c.random(m);}
};

}  // namespace oee::geb
