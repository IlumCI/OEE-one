// Port of Geb v08b3 geb_rs: component (production rule) abundance records for the
// real and shadow populations. See docs/GEB_SPEC.md section 7.
#pragma once

#include <cstdio>
#include <string>
#include <unordered_set>
#include <vector>

#include "geb/world.hpp"

namespace oee::geb {

class RuleStat {
 public:
  // dictionary_file may be null; if set, each component name is written once with
  // its genes ("<time> <R|S> <name> <genes>"), for analysis under other groupings.
  RuleStat(bool reset_shadow, int every, std::FILE* rulestat, std::FILE* shadow_rulestat,
           std::FILE* verifier, std::FILE* shadow_verifier, std::FILE* stdout_file,
           std::FILE* dictionary_file);

  void write_headers();
  void snapshot(World& w);

 private:
  struct Component {
    std::vector<Gene> genes;
    int abundance = 0;
    int shadow_abundance = 0;
  };
  struct Bucket {
    std::vector<Component> list;
    int shadow_from = -1;  // index of first shadow-only component (resets variant)
  };
  struct Stats {
    int max_count = 0, max_sum_len = 0, sum_count = 0, sum_sum_len = 0;
    int max_used = 0, max_sum_used_len = 0, sum_used = 0, sum_sum_used_len = 0;
  };

  static void grow(std::vector<Bucket>& b, int length, bool reset_markers);
  // Returns index of the component in bucket, appending if absent.
  static int find_or_add(Bucket& bucket, const Rule& rule, bool& added);
  void count_individual(const std::vector<Rule>& rules, Stats& s, std::vector<Bucket>& buckets,
                        bool shadow_pass);
  void print_complexities(long t, const Stats& s, double n, bool shadow);
  void emit_dictionary(long t, char pop, const std::string& name, const std::vector<Gene>& g);

  bool reset_shadow_;
  int every_;
  std::FILE *rulestat_, *shadow_rulestat_, *verifier_, *shadow_verifier_, *out_, *dict_;
  std::vector<Bucket> buckets_;         // real (and, with resets, shared)
  std::vector<Bucket> shadow_buckets_;  // no-resets variant only
  std::unordered_set<std::string> dict_seen_;
};

}  // namespace oee::geb
