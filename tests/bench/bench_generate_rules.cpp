// Benchmark and equivalence check for generate_rules: compares the library
// implementation against the original (reference) implementation on every genotype
// in a checkpoint (real agents and both shadows), and times both.
#include <chrono>
#include <cstdio>
#include <cstring>
#include <vector>

#include "geb/ann.hpp"
#include "geb/rng.hpp"
#include "geb/rulestat.hpp"
#include "geb/serial.hpp"
#include "geb/world.hpp"

namespace oee::geb {
namespace ref {
namespace {
struct ReadRule {
  bool ok = false;
  Rule rule;
};

// try_to_read_rule: `from` must satisfy g[from] == g[from+1] == 1.
ReadRule try_to_read_rule(const Genotype& g, int from) {
  ReadRule out;
  if (!(g[from] && g[from + 1])) return out;
  const int last = static_cast<int>(g.size()) - 1;
  from += 2;
  Rule r;
  r.gene_string = GeneSpan{g.data(), from, -1};
  r.predecessor = segment(g, from);
  from = r.predecessor.last + 2;
  r.successor_replacement = segment(g, from);
  from = r.successor_replacement.last + 2;
  if (r.successor_replacement.length() == 0) r.successor_replacement = GeneSpan{};
  r.successor_new = segment(g, from);
  from = r.successor_new.last + 2;
  if (r.successor_new.length() == 0) r.successor_new = GeneSpan{};
  if (from + 5 > last) return out;
  r.reverse_inputs_type_on_new = g[from];
  r.reverse_outputs_type_on_new = g[from + 1];
  r.inhibitory_replacement_to_new = g[from + 2];
  r.excitatory_replacement_to_new = g[from + 3];
  r.inhibitory_new_to_replacement = g[from + 4];
  r.excitatory_new_to_replacement = g[from + 5];
  r.gene_string.last = from + 5;
  out.ok = true;
  out.rule = r;
  return out;
}

struct PossiblePredecessor {
  GeneSpan predecessor;
  bool has_rule = false;
  Rule rule;
  bool kept = false;
  bool checked = false;
  int next_kept = -1;
};

}  // namespace

#if 0
const std::vector<GeneSpan>& axiom_characters() {
  // Node-list order of the axiom network (output, hidden, input): see create_axiom_net.
  static const std::vector<GeneSpan> chars = {kAxiomOutput.span(), kAxiomHidden.span(),
                                              kAxiomInput.span()};
  return chars;
}
#endif

std::vector<Rule> generate_rules_reference(const Genotype& g, const std::vector<GeneSpan>& node_chars) {
  std::vector<Rule> result;
  const int last = static_cast<int>(g.size()) - 1;
  std::vector<PossiblePredecessor> pp;
  for (int gene = 0; (last - gene) > 9; ++gene) {
    if (g[gene] && g[gene + 1]) {
      GeneSpan pred = segment(g, gene + 2);
      if (pred.last + 9 <= last) {
        PossiblePredecessor p;
        p.predecessor = pred;
        pp.push_back(p);
      }
    }
  }
  if (pp.empty()) return result;

  int max_len = 0;
  for (const auto& p : pp) max_len = std::max(max_len, p.predecessor.length());
  std::vector<std::vector<int>> by_len(max_len + 1);
  for (int i = 0; i < static_cast<int>(pp.size()); ++i)
    by_len[pp[i].predecessor.length()].push_back(i);

  int first_kept = -1;

  auto overlaps_kept = [&](int idx) {
    const Rule& r = pp[idx].rule;
    for (int c = first_kept; c != -1; c = pp[c].next_kept) {
      if (c == idx) continue;
      const Rule& o = pp[c].rule;
      if (o.gene_string.first <= r.gene_string.last && o.gene_string.last >= r.gene_string.first)
        return true;
    }
    return false;
  };

  auto find_best = [&](const GeneSpan& character) -> int {
    int len = std::min(character.length(), max_len);
    for (; len >= 0; --len) {
      for (int idx : by_len[len]) {
        PossiblePredecessor& p = pp[idx];
        if (!p.predecessor.is_start_of(character)) continue;
        if (!p.has_rule) {
          ReadRule rr = try_to_read_rule(g, p.predecessor.first - 2);
          if (rr.ok) {
            p.has_rule = true;
            p.rule = rr.rule;
          }
        }
        if (p.has_rule && !overlaps_kept(idx)) return idx;
      }
    }
    return -1;
  };

  auto insert_kept = [&](int idx) {
    const int start = pp[idx].predecessor.first;
    if (first_kept != -1 && pp[first_kept].predecessor.first < start) {
      int prev = first_kept;
      while (pp[prev].next_kept != -1 && pp[pp[prev].next_kept].predecessor.first < start)
        prev = pp[prev].next_kept;
      pp[idx].next_kept = pp[prev].next_kept;
      pp[prev].next_kept = idx;
    } else {
      pp[idx].next_kept = first_kept;
      first_kept = idx;
    }
  };

  int num_kept = 0;
  for (const GeneSpan& ch : node_chars) {
    const int best = find_best(ch);
    if (best != -1 && !pp[best].kept) {
      pp[best].kept = true;
      ++num_kept;
      insert_kept(best);
    }
  }

  int previous = 0;
  while (num_kept != previous) {
    previous = num_kept;
    for (int k = first_kept; k != -1; k = pp[k].next_kept) {
      if (pp[k].checked) continue;
      pp[k].checked = true;
      for (int s = 0; s < 2; ++s) {
        const GeneSpan ch = s ? pp[k].rule.successor_new : pp[k].rule.successor_replacement;
        const int keep = find_best(ch);
        if (keep != -1 && !pp[keep].kept) {
          pp[keep].kept = true;
          ++num_kept;
          insert_kept(keep);
        }
      }
    }
  }

  for (const auto& p : pp)
    if (p.has_rule && p.kept) result.push_back(p.rule);
  return result;
}


}  // namespace ref
}  // namespace oee::geb

using namespace oee::geb;

int main(int argc, char** argv) {
  if (argc < 2) { std::fprintf(stderr, "usage: bench_generate_rules CHECKPOINT|--fuzz\n"); return 2; }
  if (!std::strcmp(argv[1], "--fuzz")) {
    // Random genotypes: lengths 1..400, varied gene densities; compare all rule fields.
    Ran1 rng;
    rng.seed(12345);
    long cases = 0, bad = 0, rules = 0;
    for (int len = 1; len <= 400; ++len)
      for (int dens : {10, 30, 50, 70, 90})
        for (int rep = 0; rep < 25; ++rep) {
          Genotype g(len);
          for (auto& x : g) x = rng.random(100) < dens ? 1 : 0;
          auto a = generate_rules(g, axiom_characters());
          auto e = ref::generate_rules_reference(g, axiom_characters());
          ++cases;
          rules += static_cast<long>(e.size());
          bool same = a.size() == e.size();
          for (std::size_t i = 0; same && i < a.size(); ++i) {
            const Rule &x = a[i], &y = e[i];
            auto eq = [](const GeneSpan& u, const GeneSpan& v) {
              return u.null() == v.null() && (u.null() || (u.first == v.first && u.last == v.last));
            };
            same = eq(x.gene_string, y.gene_string) && eq(x.predecessor, y.predecessor) &&
                   eq(x.successor_replacement, y.successor_replacement) &&
                   eq(x.successor_new, y.successor_new) &&
                   x.reverse_inputs_type_on_new == y.reverse_inputs_type_on_new &&
                   x.reverse_outputs_type_on_new == y.reverse_outputs_type_on_new &&
                   x.inhibitory_replacement_to_new == y.inhibitory_replacement_to_new &&
                   x.excitatory_replacement_to_new == y.excitatory_replacement_to_new &&
                   x.inhibitory_new_to_replacement == y.inhibitory_new_to_replacement &&
                   x.excitatory_new_to_replacement == y.excitatory_new_to_replacement;
          }
          if (!same) ++bad;
        }
    std::printf("fuzz cases=%ld rules=%ld mismatches=%ld\n", cases, rules, bad);
    return bad ? 1 : 0;
  }
  std::FILE* f = std::fopen(argv[1], "rb");
  if (!f) return 2;
  std::vector<unsigned char> b;
  unsigned char buf[1 << 16];
  std::size_t n;
  while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) b.insert(b.end(), buf, buf + n);
  std::fclose(f);
  b.resize(b.size() - 8);
  Reader r(b.data(), b.size());
  r.get<std::uint64_t>();
  r.get_str();
  auto w = World::load(r);
  std::vector<Genotype> gs;
  for (int c = 0; c < w->world_size(); ++c)
    if (w->at(c)) gs.push_back(w->at(c)->genotype());
  for (int k = 0; k < w->shadow_count(); ++k) gs.push_back(w->shadow_genotype(k));
  for (const auto& g : w->second_shadow()) gs.push_back(g);
  std::size_t mism = 0, total_rules = 0;
  for (const auto& g : gs) {
    auto a = generate_rules(g, axiom_characters());
    auto e = ref::generate_rules_reference(g, axiom_characters());
    total_rules += e.size();
    bool same = a.size() == e.size();
    for (std::size_t i = 0; same && i < a.size(); ++i)
      same = a[i].gene_string.first == e[i].gene_string.first &&
             a[i].gene_string.last == e[i].gene_string.last &&
             a[i].predecessor.first == e[i].predecessor.first &&
             a[i].predecessor.last == e[i].predecessor.last &&
             a[i].successor_replacement.length() == e[i].successor_replacement.length() &&
             a[i].successor_new.length() == e[i].successor_new.length();
    if (!same) ++mism;
  }
  auto time = [&](auto fn) {
    auto t0 = std::chrono::steady_clock::now();
    std::size_t s = 0;
    for (int rep = 0; rep < 5; ++rep)
      for (const auto& g : gs) s += fn(g).size();
    auto t1 = std::chrono::steady_clock::now();
    return std::make_pair(std::chrono::duration<double, std::micro>(t1 - t0).count() / (5.0 * gs.size()), s);
  };
  auto [tn, sn] = time([](const Genotype& g) { return generate_rules(g, axiom_characters()); });
  auto [tr, sr] = time([](const Genotype& g) { return ref::generate_rules_reference(g, axiom_characters()); });
  std::size_t mean_len = 0;
  for (const auto& g : gs) mean_len += g.size();
  std::printf("genotypes=%zu mean_len=%zu rules=%zu mismatches=%zu new=%.2fus ref=%.2fus speedup=%.2fx\n",
              gs.size(), mean_len / gs.size(), total_rules, mism, tn, tr, tr / tn);
  return mism ? 1 : 0;
}
