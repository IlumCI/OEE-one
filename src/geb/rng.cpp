#include "geb/rng.hpp"

#include <cmath>
#include <ctime>

namespace oee::geb {

namespace {
constexpr int M1 = 259200, IA1 = 7141, IC1 = 54773;
constexpr int M2 = 134456, IA2 = 8121, IC2 = 28411;
constexpr int M3 = 243000, IA3 = 4561, IC3 = 51349;
constexpr double RM1 = 1.0 / M1;
constexpr double RM2 = 1.0 / M2;
}  // namespace

int Ran1::seed(int s) {
  if (s == 0) s = static_cast<int>(std::time(nullptr) % 32767);
  int idum = -s;
  // Original: ran1(&idum) with idum < 0 initialises the table and draws once.
  iff_ = 1;
  ix1_ = (IC1 - idum) % M1;
  ix1_ = (IA1 * ix1_ + IC1) % M1;
  ix2_ = ix1_ % M2;
  ix1_ = (IA1 * ix1_ + IC1) % M1;
  ix3_ = ix1_ % M3;
  for (int j = 1; j <= 97; ++j) {
    ix1_ = (IA1 * ix1_ + IC1) % M1;
    ix2_ = (IA2 * ix2_ + IC2) % M2;
    r_[j] = static_cast<float>((ix1_ + ix2_ * RM2) * RM1);
  }
  (void)next();
  return s;
}

float Ran1::next() {
  ix1_ = (IA1 * ix1_ + IC1) % M1;
  ix2_ = (IA2 * ix2_ + IC2) % M2;
  ix3_ = (IA3 * ix3_ + IC3) % M3;
  const int j = 1 + ((97 * ix3_) / M3);
  const float temp = r_[j];
  r_[j] = static_cast<float>((ix1_ + ix2_ * RM2) * RM1);
  return temp;
}

double Ran1::randval(double limit) { return limit * static_cast<double>(next()); }

int Ran1::random(int m) {
  if (m == 0) return 0;
  int ir = m;
  while (ir == m) ir = static_cast<int>(std::floor(randval(static_cast<double>(m))));
  return ir;
}

}  // namespace oee::geb
