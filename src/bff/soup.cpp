#include "bff/soup.hpp"

#include <brotli/encode.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <thread>

namespace oee::bff {

namespace {
enum Op : std::uint8_t { kLoopStart, kLoopEnd, kPlus, kMinus, kCopy01, kCopy10, kDec0, kInc0, kDec1, kInc1, kNull, kNoop };

struct OpTable {
  Op t[256];
  constexpr OpTable() : t{} {
    for (int i = 0; i < 256; ++i) t[i] = kNoop;
    t[static_cast<int>('[')] = kLoopStart;
    t[static_cast<int>(']')] = kLoopEnd;
    t[static_cast<int>('+')] = kPlus;
    t[static_cast<int>('-')] = kMinus;
    t[static_cast<int>('.')] = kCopy01;
    t[static_cast<int>(',')] = kCopy10;
    t[static_cast<int>('<')] = kDec0;
    t[static_cast<int>('>')] = kInc0;
    t[static_cast<int>('{')] = kDec1;
    t[static_cast<int>('}')] = kInc1;
    t[0] = kNull;
  }
};
constexpr OpTable kOps;
}  // namespace

std::size_t evaluate(std::uint8_t* tape, std::size_t max_steps) {
  // cubff bff_noheads: heads start at 2*kTape (== 0 after masking), pc at 0.
  int pc = 0, h0 = kPair, h1 = kPair;
  std::size_t skipped = 0, i = 0;
  for (; i < max_steps; ++i) {
    h0 &= kPair - 1;
    h1 &= kPair - 1;
    switch (kOps.t[tape[pc]]) {
      case kDec0: --h0; break;
      case kInc0: ++h0; break;
      case kDec1: --h1; break;
      case kInc1: ++h1; break;
      case kPlus: ++tape[h0]; break;
      case kMinus: --tape[h0]; break;
      case kCopy01: tape[h1] = tape[h0]; break;
      case kCopy10: tape[h0] = tape[h1]; break;
      case kLoopStart:
        if (tape[h0] == 0) {
          int depth = 1;
          ++pc;
          for (; pc < kPair && depth > 0; ++pc) {
            const Op o = kOps.t[tape[pc]];
            if (o == kLoopEnd) --depth;
            if (o == kLoopStart) ++depth;
          }
          --pc;
          if (depth != 0) pc = kPair;
        }
        break;
      case kLoopEnd:
        if (tape[h0] != 0) {
          int depth = 1;
          --pc;
          for (; pc >= 0 && depth > 0; --pc) {
            const Op o = kOps.t[tape[pc]];
            if (o == kLoopEnd) ++depth;
            if (o == kLoopStart) --depth;
          }
          ++pc;
          if (depth != 0) pc = -1;
        }
        break;
      default: ++skipped; break;
    }
    if (pc < 0) { ++i; break; }
    if (++pc >= kPair) { ++i; break; }
  }
  return i - skipped;
}

Soup::Soup(const Params& p) : p_(p), soup_(p.num_programs * kTape), order_(p.num_programs) {
  if (p_.num_programs % 2) throw std::invalid_argument("num_programs must be even");
  const std::uint64_t s0 = seed(0), n = p_.num_programs;
  for (std::uint64_t idx = 0; idx < n; ++idx)
    for (std::uint64_t i = 0; i < kTape; ++i)
      soup_[idx * kTape + i] = static_cast<std::uint8_t>(splitmix64(kTape * n * s0 + kTape * idx + i) % 256);
}

void Soup::run_pairs(std::size_t begin, std::size_t end, std::uint64_t epoch_seed, std::uint64_t* ops) {
  std::uint8_t tape[kPair];
  const std::uint64_t n = p_.num_programs;
  std::uint64_t total = 0;
  for (std::size_t index = begin; index < end; ++index) {
    const std::uint32_t a = order_[2 * index], b = order_[2 * index + 1];
    std::memcpy(tape, &soup_[std::size_t(a) * kTape], kTape);
    std::memcpy(tape + kTape, &soup_[std::size_t(b) * kTape], kTape);
    for (std::uint64_t i = 0; i < kPair; ++i) {
      const std::uint64_t r = splitmix64((n * epoch_seed + index) * kPair + i);
      if (((r >> 8) & ((1ULL << 30) - 1)) < p_.mutation_prob) tape[i] = static_cast<std::uint8_t>(r & 0xFF);
    }
    total += evaluate(tape);
    std::memcpy(&soup_[std::size_t(a) * kTape], tape, kTape);
    std::memcpy(&soup_[std::size_t(b) * kTape], tape + kTape, kTape);
  }
  *ops = total;
}

void Soup::epoch() {
  const std::size_t n = p_.num_programs;
  for (std::size_t i = 0; i < n; ++i) order_[i] = static_cast<std::uint32_t>(i);
  for (std::size_t i = n; i-- > 0;) {
    const std::size_t j = splitmix64(seed(epoch_ * n + i)) % (i + 1);
    std::swap(order_[i], order_[j]);
  }
  const std::uint64_t es = seed(epoch_);
  const std::size_t pairs = n / 2;
  const int T = std::max(1, p_.threads);
  std::vector<std::uint64_t> ops(T, 0);
  if (T == 1) {
    run_pairs(0, pairs, es, &ops[0]);
  } else {
    std::vector<std::thread> th;
    for (int t = 0; t < T; ++t)
      th.emplace_back([&, t] { run_pairs(pairs * t / T, pairs * (t + 1) / T, es, &ops[t]); });
    for (auto& x : th) x.join();
  }
  std::uint64_t tot = 0;
  for (auto o : ops) tot += o;
  last_ops_per_run_ = double(tot) / pairs;
  ++epoch_;
}

Metrics Soup::metrics() const {
  std::vector<std::uint8_t> out(BrotliEncoderMaxCompressedSize(soup_.size()));
  std::size_t sz = out.size();
  if (!BrotliEncoderCompress(2, 24, BROTLI_MODE_GENERIC, soup_.size(), soup_.data(), &sz, out.data()))
    throw std::runtime_error("brotli failed");
  std::size_t counts[256] = {};
  for (auto c : soup_) ++counts[c];
  double h0 = 0;
  for (auto c : counts)
    if (c) {
      const double f = double(c) / soup_.size();
      h0 -= f * std::log2(f);
    }
  const double bpb = sz * 8.0 / soup_.size();
  return Metrics{epoch_, sz, h0, bpb, h0 - bpb, last_ops_per_run_};
}

void Soup::save(const std::string& path) const {
  const std::string tmp = path + ".tmp";
  std::FILE* f = std::fopen(tmp.c_str(), "wb");
  if (!f) throw std::runtime_error("cannot write " + tmp);
  const std::uint64_t hdr[5] = {0x4f45454246460001ULL, p_.num_programs, p_.seed, p_.mutation_prob, epoch_};
  bool ok = std::fwrite(hdr, sizeof hdr, 1, f) == 1 && std::fwrite(soup_.data(), 1, soup_.size(), f) == soup_.size();
  ok = (std::fclose(f) == 0) && ok;
  if (!ok || std::rename(tmp.c_str(), path.c_str()) != 0) throw std::runtime_error("cannot save " + path);
}

Soup Soup::load(const std::string& path, int threads) {
  std::FILE* f = std::fopen(path.c_str(), "rb");
  if (!f) throw std::runtime_error("cannot read " + path);
  std::uint64_t hdr[5];
  if (std::fread(hdr, sizeof hdr, 1, f) != 1 || hdr[0] != 0x4f45454246460001ULL) {
    std::fclose(f);
    throw std::runtime_error("bad checkpoint " + path);
  }
  Params p;
  p.num_programs = hdr[1];
  p.seed = hdr[2];
  p.mutation_prob = static_cast<std::uint32_t>(hdr[3]);
  p.threads = threads;
  Soup s(p);
  const bool ok = std::fread(s.soup_.data(), 1, s.soup_.size(), f) == s.soup_.size();
  std::fclose(f);
  if (!ok) throw std::runtime_error("truncated checkpoint " + path);
  s.epoch_ = hdr[4];
  return s;
}

}  // namespace oee::bff
