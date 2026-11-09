// Inspect a BFF checkpoint: self-replicator count (cubff CheckSelfRep criterion) and the
// most common programs. Usage: bff_inspect CHECKPOINT [TOP] [THREADS]
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include "bff/soup.hpp"

using namespace oee::bff;

int main(int argc, char** argv) {
  if (argc < 2) return 2;
  const int top = argc > 2 ? std::atoi(argv[2]) : 10;
  const int T = argc > 3 ? std::atoi(argv[3]) : 1;
  Soup s = Soup::load(argv[1], 1);
  const std::size_t n = s.params().num_programs;
  const std::uint64_t seed = s.epoch_seed(s.epochs_done() - 1);
  std::vector<std::size_t> score(n);
  std::vector<std::thread> th;
  for (int t = 0; t < T; ++t)
    th.emplace_back([&, t] {
      for (std::size_t i = n * t / T; i < n * (t + 1) / T; ++i)
        score[i] = selfrep_score(&s.data()[i * kTape], seed, n, i);
    });
  for (auto& x : th) x.join();
  std::size_t reps = 0;
  for (auto v : score) reps += v >= kSelfrepThreshold;
  std::map<std::string, std::pair<std::size_t, std::size_t>> freq;  // text -> (count, selfrep count)
  for (std::size_t i = 0; i < n; ++i) {
    auto& f = freq[to_text(&s.data()[i * kTape])];
    ++f.first;
    f.second += score[i] >= kSelfrepThreshold;
  }
  std::vector<std::pair<std::size_t, std::string>> v;
  for (auto& [k, c] : freq) v.push_back({c.first, k});
  std::sort(v.rbegin(), v.rend());
  const auto m = s.metrics();
  std::printf("{\"epoch\": %zu, \"programs\": %zu, \"selfreplicators\": %zu, \"distinct\": %zu, "
              "\"higher_entropy\": %.4f, \"top\": [", s.epochs_done(), n, reps, freq.size(), m.higher_entropy);
  for (int i = 0; i < top && i < (int)v.size(); ++i)
    std::printf("%s{\"count\": %zu, \"selfrep\": %zu, \"program\": \"%s\"}", i ? ", " : "", v[i].first,
                freq[v[i].second].second, v[i].second.c_str());
  std::printf("]}\n");
}
