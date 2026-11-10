#include "bff/soup.hpp"

#include <brotli/encode.h>

#include <algorithm>
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

std::size_t selfrep_score(const std::uint8_t* program, std::uint64_t seed, std::size_t num_programs,
                          std::size_t index) {
  constexpr std::size_t kIters = 13, kExtraGens = 4;
  std::uint8_t tapes[kIters][kPair] = {};
  const std::uint64_t local = splitmix64(num_programs * seed + index);
  for (std::size_t i = 0; i < kIters; ++i) {
    std::uint8_t noise[kTape];
    for (int j = 0; j < kTape; ++j) noise[j] = splitmix64(local ^ splitmix64((i + 1) * kTape + j)) % 256;
    std::uint8_t* t = tapes[i];
    std::memcpy(t, program, kTape);
    std::memcpy(t + kTape, noise, kTape);
    evaluate(t);
    for (std::size_t g = 0; g < kExtraGens; ++g) {
      std::memcpy(t, t + kTape, kTape);
      std::memcpy(t + kTape, noise, kTape);
      evaluate(t);
    }
  }
  std::size_t res[2] = {};
  for (int i = 0; i < kPair; ++i)
    for (std::size_t a = 0; a < kIters; ++a) {
      std::size_t count = 1;
      if (i < kTape && tapes[a][i] != program[i]) continue;
      for (std::size_t b = a + 1; b < kIters; ++b)
        if (tapes[a][i] == tapes[b][i]) ++count;
      if (count > kIters / 4) {
        ++res[i / kTape];
        break;
      }
    }
  return std::min(res[0], res[1]);
}

std::string to_text(const std::uint8_t* program, std::size_t n) {
  std::string s;
  for (std::size_t i = 0; i < n; ++i) {
    const Op o = kOps.t[program[i]];
    s += o == kNull ? '0' : o == kNoop ? '_' : static_cast<char>(program[i]);
  }
  return s;
}

void Soup::build_neighbours() {
  // Same order as cubff make_2d_pattern.py: rows, then dy, then dx; self excluded.
  const int W = p_.grid_w, H = p_.grid_h, R = p_.radius;
  neigh_.assign(p_.num_programs, {});
  for (int i = 0; i < H; ++i)
    for (int j = 0; j < W; ++j)
      for (int di = -R; di <= R; ++di)
        for (int dj = -R; dj <= R; ++dj) {
          const int y = i + di, x = j + dj;
          if (y >= 0 && y < H && x >= 0 && x < W && (di || dj)) neigh_[i * W + j].push_back(y * W + x);
        }
  used_.assign(p_.num_programs, 0);
}

void Soup::spatial_pairs() {
  // cubff allowed_interactions branch: visit programs in shuffled order; each picks a
  // random neighbour; pair if neither is taken. Unpaired programs follow and are only
  // mutated.
  const std::size_t n = p_.num_programs;
  std::vector<std::uint32_t> visit(n);
  for (std::size_t i = 0; i < n; ++i) visit[i] = static_cast<std::uint32_t>(i);
  for (std::size_t i = n; i-- > 0;) {
    const std::size_t j = splitmix64(seed(epoch_ * n + i)) % (i + 1);
    std::swap(visit[i], visit[j]);
  }
  std::fill(used_.begin(), used_.end(), 0);
  std::size_t k = 0;
  for (std::uint32_t i : visit) {
    const auto& nb = neigh_[i];
    if (nb.empty()) continue;
    const std::uint32_t other = nb[seed(seed(epoch_) ^ seed(i)) % nb.size()];
    if (used_[i] || used_[other]) continue;
    used_[i] = used_[other] = 1;
    order_[2 * k] = i;
    order_[2 * k + 1] = other;
    ++k;
  }
  std::size_t idx = 2 * k;
  for (std::size_t i = 0; i < n; ++i)
    if (!used_[i]) order_[idx++] = static_cast<std::uint32_t>(i);
  active_pairs_ = k;
}

std::size_t evaluate_n(std::uint8_t* tape, int n, std::size_t max_steps) {
  if (n <= 0) return 0;
  auto wrap = [n](int h) { h %= n; return h < 0 ? h + n : h; };
  int pc = 0, h0 = 0, h1 = 0;
  std::size_t skipped = 0, i = 0;
  for (; i < max_steps; ++i) {
    h0 = wrap(h0);
    h1 = wrap(h1);
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
          for (; pc < n && depth > 0; ++pc) {
            const Op o = kOps.t[tape[pc]];
            if (o == kLoopEnd) --depth;
            if (o == kLoopStart) ++depth;
          }
          --pc;
          if (depth != 0) pc = n;
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
    if (++pc >= n) { ++i; break; }
  }
  return i - skipped;
}

Soup::Soup(const Params& p) : p_(p), soup_(p.num_programs * kTape), order_(p.num_programs) {
  if (p_.grid_w) {
    if (std::size_t(p_.grid_w) * p_.grid_h != p_.num_programs)
      throw std::invalid_argument("grid_w * grid_h must equal num_programs");
    build_neighbours();
  }
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
    if (p_.m5_protocol) {
      for (int i = 0; i < kTape; ++i)
        tape[kTape + i] = static_cast<std::uint8_t>(splitmix64(~((n * epoch_seed + index) * kPair + i)) & 0xFF);
    } else {
      std::memcpy(tape + kTape, &soup_[std::size_t(b) * kTape], kTape);
    }
    for (std::uint64_t i = 0; i < kPair; ++i) {
      const std::uint64_t r = splitmix64((n * epoch_seed + index) * kPair + i);
      if (((r >> 8) & ((1ULL << 30) - 1)) < p_.mutation_prob) tape[i] = static_cast<std::uint8_t>(r & 0xFF);
    }
    if (index < active_pairs_) total += evaluate(tape);
    if (p_.m5_protocol) {
      std::memcpy(&soup_[std::size_t(b) * kTape], tape + kTape, kTape);
    } else {
      std::memcpy(&soup_[std::size_t(a) * kTape], tape, kTape);
      std::memcpy(&soup_[std::size_t(b) * kTape], tape + kTape, kTape);
    }
  }
  *ops = total;
}

void Soup::epoch() {
  const std::size_t n = p_.num_programs;
  if (p_.grid_w) {
    spatial_pairs();
  } else {
    for (std::size_t i = 0; i < n; ++i) order_[i] = static_cast<std::uint32_t>(i);
    for (std::size_t i = n; i-- > 0;) {
      const std::size_t j = splitmix64(seed(epoch_ * n + i)) % (i + 1);
      std::swap(order_[i], order_[j]);
    }
    active_pairs_ = n / 2;
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
  last_ops_per_run_ = active_pairs_ ? double(tot) / active_pairs_ : 0.0;
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
  const std::uint64_t hdr[6] = {0x4f45454246460002ULL, p_.num_programs, p_.seed, p_.mutation_prob, epoch_,
                                std::uint64_t(p_.grid_w) | std::uint64_t(p_.grid_h) << 24 | std::uint64_t(p_.radius) << 48 |
                                    std::uint64_t(p_.m5_protocol) << 63};
  bool ok = std::fwrite(hdr, sizeof hdr, 1, f) == 1 && std::fwrite(soup_.data(), 1, soup_.size(), f) == soup_.size();
  ok = (std::fclose(f) == 0) && ok;
  if (!ok || std::rename(tmp.c_str(), path.c_str()) != 0) throw std::runtime_error("cannot save " + path);
}

Soup Soup::load(const std::string& path, int threads) {
  std::FILE* f = std::fopen(path.c_str(), "rb");
  if (!f) throw std::runtime_error("cannot read " + path);
  std::uint64_t hdr[6] = {};
  if (std::fread(hdr, 8 * 5, 1, f) != 1 ||
      (hdr[0] != 0x4f45454246460001ULL && hdr[0] != 0x4f45454246460002ULL) ||
      (hdr[0] == 0x4f45454246460002ULL && std::fread(&hdr[5], 8, 1, f) != 1)) {
    std::fclose(f);
    throw std::runtime_error("bad checkpoint " + path);
  }
  Params p;
  p.num_programs = hdr[1];
  p.seed = hdr[2];
  p.mutation_prob = static_cast<std::uint32_t>(hdr[3]);
  p.threads = threads;
  p.grid_w = static_cast<int>(hdr[5] & 0xFFFFFF);
  p.grid_h = static_cast<int>(hdr[5] >> 24 & 0xFFFFFF);
  if (hdr[0] == 0x4f45454246460002ULL) {
    p.radius = static_cast<int>(hdr[5] >> 48 & 0x7FFF);
    p.m5_protocol = hdr[5] >> 63;
  }
  Soup s(p);
  const bool ok = std::fread(s.soup_.data(), 1, s.soup_.size(), f) == s.soup_.size();
  std::fclose(f);
  if (!ok) throw std::runtime_error("truncated checkpoint " + path);
  s.epoch_ = hdr[4];
  return s;
}

}  // namespace oee::bff
