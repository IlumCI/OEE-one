#include "geb/rulestat.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace oee::geb {

namespace {
constexpr int kInitialMaxComponentLength = 10;

std::uint64_t fnv(std::uint64_t h, Gene g) {
  h ^= static_cast<std::uint64_t>(static_cast<unsigned char>(g));
  return h * 1099511628211ULL;
}
constexpr std::uint64_t kFnvOffset = 1469598103934665603ULL;

std::pair<std::uint64_t, std::uint64_t> component_hashes(const Rule& r) {
  std::uint64_t hr = kFnvOffset;
  for (int k = 0; k < r.gene_string.length(); ++k) hr = fnv(hr, r.gene_string.at(k));
  std::uint64_t hp = kFnvOffset;
  for (int k = 0; k < r.predecessor.length(); ++k) hp = fnv(hp, r.predecessor.at(k));
  hp = fnv(hp, 2);  // separator
  const Gene links[6] = {r.reverse_inputs_type_on_new,    r.reverse_outputs_type_on_new,
                         r.inhibitory_replacement_to_new, r.excitatory_replacement_to_new,
                         r.inhibitory_new_to_replacement, r.excitatory_new_to_replacement};
  for (Gene g : links) hp = fnv(hp, g);
  return {hr, hp};
}
constexpr int kIncrement = 10;
}  // namespace

RuleStat::RuleStat(bool reset_shadow, int every, std::FILE* rulestat, std::FILE* shadow_rulestat,
                   std::FILE* verifier, std::FILE* shadow_verifier, std::FILE* stdout_file,
                   std::FILE* dictionary_file)
    : reset_shadow_(reset_shadow),
      every_(every),
      rulestat_(rulestat),
      shadow_rulestat_(shadow_rulestat),
      verifier_(verifier),
      shadow_verifier_(shadow_verifier),
      out_(stdout_file),
      dict_(dictionary_file) {
  buckets_.resize(1 + kInitialMaxComponentLength);
  if (!reset_shadow_) shadow_buckets_.resize(1 + kInitialMaxComponentLength);
}

void RuleStat::write_headers() {
  std::fprintf(rulestat_, "Geb runtime production rule abundance statistics\n");
  std::fprintf(rulestat_, "Output every %d timesteps\n", every_);
  std::fprintf(rulestat_, "For processing by actiview\n");
  std::fprintf(rulestat_, "= DATA =\n");
  std::fprintf(rulestat_, "component data format: population\n");
  std::fprintf(shadow_rulestat_, "Geb SHADOWruntime production rule abundance statistics\n");
  std::fprintf(shadow_rulestat_, "Output every %d timesteps\n", every_);
  std::fprintf(shadow_rulestat_, "For processing by actiview\n");
  std::fprintf(shadow_rulestat_, "= DATA =\n");
  std::fprintf(shadow_rulestat_, "component data format: population\n");
}

void RuleStat::grow(std::vector<Bucket>& b, int length, bool reset_markers) {
  int max_len = static_cast<int>(b.size()) - 1;
  if (length <= max_len) return;
  while (length > max_len) max_len += kIncrement;
  b.resize(1 + max_len);
  // Original quirk: growth copies only list heads; shadow_components_from markers are lost.
  if (reset_markers)
    for (Bucket& bk : b) bk.shadow_from = -1;
}

int RuleStat::find_or_add(Bucket& bucket, const Rule& rule, bool& added) {
  const GeneSpan& gs = rule.gene_string;
  const int n = gs.length();
  for (int i = 0; i < static_cast<int>(bucket.list.size()); ++i) {
    const auto& g = bucket.list[i].genes;
    bool eq = true;
    for (int k = 0; k < n; ++k)
      if (g[k] != gs.at(k)) {
        eq = false;
        break;
      }
    if (eq) {
      added = false;
      return i;
    }
  }
  Component c;
  c.genes.resize(n);
  for (int k = 0; k < n; ++k) c.genes[k] = gs.at(k);
  bucket.list.push_back(std::move(c));
  added = true;
  return static_cast<int>(bucket.list.size()) - 1;
}

void RuleStat::count_individual(const std::vector<Rule>& rules, Stats& s,
                                std::vector<Bucket>& buckets, bool shadow_pass) {
  int count = 0, sum_len = 0, used = 0, sum_used_len = 0;
  for (const Rule& r : rules) {
    const int len = r.gene_string.length();
    ++count;
    sum_len += len;
    if (r.used) {
      ++used;
      sum_used_len += len;
    }
    grow(buckets, len, reset_shadow_ && shadow_pass);
    Bucket& b = buckets[len];
    bool added = false;
    const int idx = find_or_add(b, r, added);
    Component& c = b.list[idx];
    if (shadow_pass && reset_shadow_) {
      if (added) {
        c.shadow_abundance = 1;
        c.abundance = 0;
        if (b.shadow_from == -1) b.shadow_from = idx;
      } else {
        ++c.shadow_abundance;
      }
    } else {
      if (added) {
        c.abundance = 1;
        c.shadow_abundance = 0;
      } else {
        ++c.abundance;
      }
    }
  }
  if (count > s.max_count) s.max_count = count;
  s.sum_count += count;
  if (sum_len > s.max_sum_len) s.max_sum_len = sum_len;
  s.sum_sum_len += sum_len;
  if (used > s.max_used) s.max_used = used;
  s.sum_used += used;
  if (sum_used_len > s.max_sum_used_len) s.max_sum_used_len = sum_used_len;
  s.sum_sum_used_len += sum_used_len;
}

void RuleStat::print_complexities(long t, const Stats& s, double n, bool shadow) {
  const char* p = shadow ? "SHADOW" : "";
  std::fprintf(out_,
               "Complexities at time=%ld: %smax_component_count=%d %smean_component_count=%lf "
               "%smax_sum_component_lengths=%d %smean_sum_component_lengths=%lf\n",
               t, p, s.max_count, p, s.sum_count / n, p, s.max_sum_len, p, s.sum_sum_len / n);
  std::fprintf(out_,
               "Complexities of used components at time=%ld: %smax_used_component_count=%d "
               "%smean_used_component_count=%lf %smax_sum_used_component_lengths=%d "
               "%smean_sum_used_component_lengths=%lf\n",
               t, p, s.max_used, p, s.sum_used / n, p, s.max_sum_used_len, p,
               s.sum_sum_used_len / n);
}

void RuleStat::emit_dictionary(long t, char pop, const std::string& name,
                               const std::vector<Gene>& g) {
  if (!dict_ || !dict_seen_.insert(name).second) return;
  std::fprintf(dict_, "%ld %c %s ", t, pop, name.c_str());
  for (Gene x : g) std::fputc('0' + x, dict_);
  std::fputc('\n', dict_);
}

void RuleStat::write_presence(long t, const char* pop,
                              const std::vector<const std::vector<Rule>*>& sets) {
  std::vector<std::pair<std::uint64_t, std::uint64_t>> v;
  for (const auto* rules : sets)
    for (const Rule& r : *rules) v.push_back(component_hashes(r));
  std::sort(v.begin(), v.end());
  v.erase(std::unique(v.begin(), v.end()), v.end());
  std::fprintf(presence_, "%ld %s %zu", t, pop, v.size());
  for (const auto& [a, b] : v)
    std::fprintf(presence_, " %016llx:%016llx", static_cast<unsigned long long>(a),
                 static_cast<unsigned long long>(b));
  std::fputc('\n', presence_);
}

void RuleStat::snapshot(World& w) {
  const long t = w.time();
  // Strip previous shadow-only components (resets variant).
  if (reset_shadow_)
    for (Bucket& b : buckets_)
      if (b.shadow_from != -1) {
        b.list.resize(b.shadow_from);
        b.shadow_from = -1;
      }
  for (Bucket& b : buckets_)
    for (Component& c : b.list) c.abundance = c.shadow_abundance = 0;

  // Real population.
  Stats rs;
  int n_real = 0;
  for (int at = 0; at < w.world_size(); ++at) {
    const Organism* o = w.at(at);
    if (!o) continue;
    ++n_real;
    count_individual(o->rules(), rs, buckets_, false);
  }
  print_complexities(t, rs, static_cast<double>(n_real), false);
  std::fprintf(rulestat_, "%ld:", t);
  std::fprintf(verifier_, "%ld:", t);
  long v0 = 0, v1 = 0, v2 = 0, v3 = 0;
  for (int len = 0; len < static_cast<int>(buckets_.size()); ++len) {
    const Bucket& b = buckets_[len];
    for (int i = 0; i < static_cast<int>(b.list.size()); ++i)
      if (b.list[i].abundance) {
        std::fprintf(rulestat_, " %d.%d", len, i + 1);
        std::fprintf(rulestat_, " %d;", b.list[i].abundance);
        v0 += len;
        v1 += i + 1;
        ++v2;
        v3 += b.list[i].abundance;
        if (dict_) emit_dictionary(t, 'R', std::to_string(len) + "." + std::to_string(i + 1),
                                   b.list[i].genes);
      }
  }
  std::fprintf(rulestat_, "\n");
  std::fflush(rulestat_);
  std::fprintf(verifier_, " %ld %ld %ld %ld\n", v0, v1, v2, v3);
  std::fflush(verifier_);

  // Shadow population.
  std::vector<Bucket>& sb = reset_shadow_ ? buckets_ : shadow_buckets_;
  if (!reset_shadow_)
    for (Bucket& b : sb)
      for (Component& c : b.list) c.abundance = 0;
  Stats ss;
  const int n_shadow = w.shadow_count();
  std::vector<std::vector<Rule>> shadow_rules;
  for (int k = 0; k < n_shadow; ++k) {
    Ann shadow(w.shadow_genotype(k), w.params().max_num_nodes);
    count_individual(shadow.rules(), ss, sb, true);
    // Copied rules keep spans into the shadow genotype owned by World (unchanged until reset).
    if (presence_) shadow_rules.push_back(generate_rules(w.shadow_genotype(k), axiom_characters()));
  }
  if (presence_) {
    std::vector<const std::vector<Rule>*> rp, sp, s2p;
    for (int at = 0; at < w.world_size(); ++at)
      if (const Organism* o = w.at(at)) rp.push_back(&o->rules());
    for (const auto& r : shadow_rules) sp.push_back(&r);
    write_presence(t, "R", rp);
    write_presence(t, "S", sp);
    if (w.params().second_shadow) {
      std::vector<std::vector<Rule>> s2;
      for (const Genotype& g : w.second_shadow()) s2.push_back(generate_rules(g, axiom_characters()));
      for (const auto& r : s2) s2p.push_back(&r);
      write_presence(t, "S2", s2p);
    }
    std::fflush(presence_);
  }
  print_complexities(t, ss, static_cast<double>(n_shadow), true);
  std::fprintf(shadow_rulestat_, "%ld:", t);
  std::fprintf(shadow_verifier_, "%ld:", t);
  long s0 = 0, s1 = 0, s2 = 0, s3 = 0;
  for (int len = 0; len < static_cast<int>(sb.size()); ++len) {
    const Bucket& b = sb[len];
    if (!reset_shadow_) {
      for (int i = 0; i < static_cast<int>(b.list.size()); ++i)
        if (b.list[i].abundance) {
          std::fprintf(shadow_rulestat_, " %d.%d", len, i + 1);
          std::fprintf(shadow_rulestat_, " %d;", b.list[i].abundance);
          s0 += len;
          s1 += i + 1;
          ++s2;
          s3 += b.list[i].abundance;
          if (dict_)
            emit_dictionary(t, 'S', "S" + std::to_string(len) + "." + std::to_string(i + 1),
                            b.list[i].genes);
        }
      continue;
    }
    const int real_end = b.shadow_from == -1 ? static_cast<int>(b.list.size()) : b.shadow_from;
    for (int i = 0; i < real_end; ++i)
      if (b.list[i].shadow_abundance) {
        std::fprintf(shadow_rulestat_, " %d.%d", len, i + 1);
        std::fprintf(shadow_rulestat_, " %d;", b.list[i].shadow_abundance);
        s0 += len;
        s1 += i + 1;
        ++s2;
        s3 += b.list[i].shadow_abundance;
        if (dict_) emit_dictionary(t, 'S', std::to_string(len) + "." + std::to_string(i + 1),
                                   b.list[i].genes);
      }
    if (b.shadow_from != -1)
      for (int i = b.shadow_from, num = 1; i < static_cast<int>(b.list.size()); ++i, ++num)
        if (b.list[i].shadow_abundance) {
          std::fprintf(shadow_rulestat_, " %d.%ds%ld", len, num, t / every_);
          std::fprintf(shadow_rulestat_, " %d;", b.list[i].shadow_abundance);
          s0 += len;
          s1 += num;
          ++s2;
          s3 += b.list[i].shadow_abundance;
          if (dict_)
            emit_dictionary(t, 'S',
                            std::to_string(len) + "." + std::to_string(num) + "s" +
                                std::to_string(t / every_),
                            b.list[i].genes);
        }
  }
  std::fprintf(shadow_rulestat_, "\n");
  std::fflush(shadow_rulestat_);
  std::fprintf(shadow_verifier_, " %ld %ld %ld %ld\n", s0, s1, s2, s3);
  std::fflush(shadow_verifier_);

  if (dict_) std::fflush(dict_);
  if (reset_shadow_) w.reset_shadow_to_real();
}

}  // namespace oee::geb

namespace oee::geb {

void RuleStat::save_buckets(Writer& w, const std::vector<Bucket>& b) {
  w.put<std::uint64_t>(b.size());
  for (const Bucket& bk : b) {
    w.put(bk.shadow_from);
    w.put<std::uint64_t>(bk.list.size());
    for (const Component& c : bk.list) {
      w.put_vec(c.genes);
      w.put(c.abundance);
      w.put(c.shadow_abundance);
    }
  }
}

void RuleStat::load_buckets(Reader& r, std::vector<Bucket>& b) {
  b.assign(r.get<std::uint64_t>(), Bucket{});
  for (Bucket& bk : b) {
    bk.shadow_from = r.get<int>();
    bk.list.resize(r.get<std::uint64_t>());
    for (Component& c : bk.list) {
      c.genes = r.get_vec<Gene>();
      c.abundance = r.get<int>();
      c.shadow_abundance = r.get<int>();
    }
  }
}

void RuleStat::save(Writer& w) const {
  w.put<std::uint8_t>(reset_shadow_ ? 1 : 0);
  save_buckets(w, buckets_);
  save_buckets(w, shadow_buckets_);
  w.put<std::uint64_t>(dict_seen_.size());
  std::vector<std::string> names(dict_seen_.begin(), dict_seen_.end());
  std::sort(names.begin(), names.end());
  for (const auto& n : names) w.put_str(n);
}

void RuleStat::load(Reader& r) {
  if ((r.get<std::uint8_t>() != 0) != reset_shadow_)
    throw std::runtime_error("checkpoint shadow variant differs from this binary/options");
  load_buckets(r, buckets_);
  load_buckets(r, shadow_buckets_);
  dict_seen_.clear();
  const auto n = r.get<std::uint64_t>();
  for (std::uint64_t i = 0; i < n; ++i) dict_seen_.insert(r.get_str());
}

}  // namespace oee::geb
