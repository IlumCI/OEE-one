// BFF primordial soup (Agüera y Arcas et al., "Computational Life", arXiv:2406.19108),
// reimplemented to match the reference implementation cubff (Apache-2.0,
// github.com/paradigms-of-intelligence/cubff, language "bff_noheads") bit for bit:
// same random streams (SplitMix64 keyed by seed, epoch and index), same interpreter,
// same pairing and mutation, same high-order-entropy metric (brotli q2, window 24).
// See docs/M3.md.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace oee::bff {

constexpr int kTape = 64;           // bytes per program
constexpr int kPair = 2 * kTape;    // concatenated interaction tape
constexpr int kMaxSteps = 8 * 1024; // instructions per interaction

inline std::uint64_t splitmix64(std::uint64_t x) {
  std::uint64_t z = x + 0x9e3779b97f4a7c15ULL;
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
  z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
  return z ^ (z >> 31);
}

// Run one interaction tape in place; returns the number of non-no-op instructions.
std::size_t evaluate(std::uint8_t* tape, std::size_t max_steps = kMaxSteps);

struct Params {
  std::size_t num_programs = 128 * 1024;
  std::uint64_t seed = 0;
  std::uint32_t mutation_prob = 1u << 18;  // out of 2^30 per byte per interaction
  int threads = 1;
};

struct Metrics {
  std::size_t epoch;        // epochs completed (cubff's state.epoch)
  std::size_t brotli_size;  // bytes
  double h0;                // byte entropy, bits per byte
  double brotli_bpb;        // compressed bits per byte
  double higher_entropy;    // h0 - brotli_bpb
  double ops_per_run;       // mean executed instructions per interaction this epoch
};

class Soup {
 public:
  explicit Soup(const Params& p);
  // Run one epoch: shuffle, pair, mutate, execute, split.
  void epoch();
  Metrics metrics() const;
  std::size_t epochs_done() const { return epoch_; }
  const std::vector<std::uint8_t>& data() const { return soup_; }
  std::vector<std::uint8_t>& data() { return soup_; }
  const Params& params() const { return p_; }

  void save(const std::string& path) const;
  static Soup load(const std::string& path, int threads);

 private:
  std::uint64_t seed(std::uint64_t x) const { return splitmix64(splitmix64(p_.seed) ^ splitmix64(x)); }
  void run_pairs(std::size_t begin, std::size_t end, std::uint64_t epoch_seed, std::uint64_t* ops);

  Params p_;
  std::vector<std::uint8_t> soup_;
  std::vector<std::uint32_t> order_;
  std::size_t epoch_ = 0;
  double last_ops_per_run_ = 0;
};

}  // namespace oee::bff
