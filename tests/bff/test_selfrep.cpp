// Positive and negative controls for the BFF self-replicator check.
#include <cstdio>
#include <cstring>
#include <string>

#include "bff/soup.hpp"

using namespace oee::bff;

int main() {
  // Palindromic copier: '{' sets head1 to 127; the loop copies tape[h0] to tape[h1]
  // with h0 ascending and h1 descending, writing the reversed program into the partner.
  // A palindrome's reversal is itself, so the partner becomes an exact copy.
  const std::string half = "{[.>{]";
  std::string p = half + std::string(kTape - 2 * half.size(), 'a') + std::string(half.rbegin(), half.rend());
  std::uint8_t prog[kTape];
  std::memcpy(prog, p.data(), kTape);
  const std::size_t s_rep = selfrep_score(prog, 12345, 1024, 7);
  std::uint8_t rnd[kTape];
  for (int i = 0; i < kTape; ++i) rnd[i] = splitmix64(99 + i) % 256;
  const std::size_t s_rnd = selfrep_score(rnd, 12345, 1024, 7);
  // Direct check: one interaction with a noise partner leaves two copies.
  std::uint8_t tape[kPair];
  std::memcpy(tape, prog, kTape);
  std::memcpy(tape + kTape, rnd, kTape);
  evaluate(tape);
  const bool copied = std::memcmp(tape, prog, kTape) == 0 && std::memcmp(tape + kTape, prog, kTape) == 0;
  std::printf("replicator %s score %zu, random score %zu, direct copy %s\n", to_text(prog).c_str(), s_rep, s_rnd,
              copied ? "yes" : "no");
  return (s_rep >= kSelfrepThreshold && s_rnd < kSelfrepThreshold && copied) ? 0 : 1;
}
