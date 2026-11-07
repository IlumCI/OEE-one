// Gene strings: spans into gene buffers. See docs/GEB_SPEC.md section 2.
#pragma once

#include <cstdint>
#include <vector>

namespace oee::geb {

using Gene = std::int8_t;
using Genotype = std::vector<Gene>;

// A span [first, last] (inclusive) into a gene buffer, or null (base == nullptr).
// length() is -1 for a null span, matching the original.
struct GeneSpan {
  const Gene* base = nullptr;
  int first = 0;
  int last = -1;

  bool null() const { return base == nullptr; }
  int length() const { return base ? last - first + 1 : -1; }
  Gene at(int k) const { return base[first + k]; }

  bool is_start_of(const GeneSpan& other) const {
    if (length() > other.length()) return false;
    if (!base) return true;
    const int n = length();
    for (int k = 0; k < n; ++k)
      if (at(k) != other.at(k)) return false;
    return true;
  }

  bool is_equal_to(const GeneSpan& other) const {
    if (length() != other.length()) return false;
    if (!base) return other.base == nullptr;
    const int n = length();
    for (int k = 0; k < n; ++k)
      if (at(k) != other.at(k)) return false;
    return true;
  }
};

// Segment reading over a genotype (get_segment in the original).
inline GeneSpan segment(const Genotype& g, int from) {
  const int last = static_cast<int>(g.size()) - 1;
  int cur = from;
  while (cur < last && g[cur] == 0) cur += 2;
  return GeneSpan{g.data(), from, cur - 1};
}

// Paired-form constant characters ("create_as_character").
struct Character {
  std::vector<Gene> genes;
  explicit Character(const char* bits) {
    for (const char* p = bits; *p; ++p) {
      genes.push_back(0);
      genes.push_back(static_cast<Gene>(1 & *p));
    }
  }
  GeneSpan span() const {
    return GeneSpan{genes.data(), 0, static_cast<int>(genes.size()) - 1};
  }
};

}  // namespace oee::geb
