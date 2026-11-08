#include "geb/ann.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>

#if defined(__x86_64__)
#include <immintrin.h>
#endif

namespace oee::geb {

namespace {

const Character kAxiomInput("001");
const Character kAxiomHidden("000");
const Character kAxiomOutput("01");

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

// ---------------------------------------------------------------------------
// Bit-stream genome scan for generate_rules.
//
// The genotype is packed into a bit array B (bit i = gene i) and two parity streams
// (S[0] bit j = gene 2j, S[1] bit j = gene 2j+1). Rule starts are positions i with
// genes i and i+1 both 1 (B & B>>1). A predecessor segment starting at p ends at the
// first position q >= p of the same parity with gene 1 and q < last, or else at the
// first same-parity position >= last; this is found with count-trailing-zeros on the
// parity stream. Results are identical to segment() (verified by bench_generate_rules).

struct Packed {
  std::vector<std::uint64_t> b, s0, s1;
};

#if defined(__x86_64__)
__attribute__((target("avx2,bmi2"))) void pack_avx2(const Genotype& g, Packed& pk) {
  const std::size_t n = g.size();
  const std::size_t words = (n + 63) / 64;
  pk.b.assign(words + 1, 0);
  pk.s0.assign(words / 2 + 2, 0);
  pk.s1.assign(words / 2 + 2, 0);
  const __m256i zero = _mm256_setzero_si256();
  std::size_t i = 0;
  for (; i + 64 <= n; i += 64) {
    const __m256i a = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(g.data() + i));
    const __m256i c = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(g.data() + i + 32));
    const std::uint32_t ma = ~static_cast<std::uint32_t>(_mm256_movemask_epi8(_mm256_cmpeq_epi8(a, zero)));
    const std::uint32_t mc = ~static_cast<std::uint32_t>(_mm256_movemask_epi8(_mm256_cmpeq_epi8(c, zero)));
    const std::uint64_t w = static_cast<std::uint64_t>(ma) | (static_cast<std::uint64_t>(mc) << 32);
    pk.b[i >> 6] = w;
    const std::uint64_t ev = _pext_u64(w, 0x5555555555555555ULL);
    const std::uint64_t od = _pext_u64(w, 0xAAAAAAAAAAAAAAAAULL);
    const std::size_t sw = (i >> 1) >> 6, sb = (i >> 1) & 63;  // sb is 0 or 32
    pk.s0[sw] |= ev << sb;
    pk.s1[sw] |= od << sb;
  }
  for (; i < n; ++i)
    if (g[i]) {
      pk.b[i >> 6] |= std::uint64_t{1} << (i & 63);
      auto& st = (i & 1) ? pk.s1 : pk.s0;
      st[(i >> 1) >> 6] |= std::uint64_t{1} << ((i >> 1) & 63);
    }
}
#endif

void pack_scalar(const Genotype& g, Packed& pk) {
  const std::size_t n = g.size();
  const std::size_t words = (n + 63) / 64;
  pk.b.assign(words + 1, 0);
  pk.s0.assign(words / 2 + 2, 0);
  pk.s1.assign(words / 2 + 2, 0);
  for (std::size_t i = 0; i < n; ++i)
    if (g[i]) {
      pk.b[i >> 6] |= std::uint64_t{1} << (i & 63);
      auto& st = (i & 1) ? pk.s1 : pk.s0;
      st[(i >> 1) >> 6] |= std::uint64_t{1} << ((i >> 1) & 63);
    }
}

bool use_avx2() {
#if defined(__x86_64__)
  static const bool ok = __builtin_cpu_supports("avx2") && __builtin_cpu_supports("bmi2") &&
                         std::getenv("OEE_FORCE_SCALAR") == nullptr;
  return ok;
#else
  return false;
#endif
}

// First set bit at index >= j in stream st (limited to `limit` bits); -1 if none.
inline int next_one(const std::vector<std::uint64_t>& st, int j, int limit) {
  if (j >= limit) return -1;
  std::size_t w = static_cast<std::size_t>(j) >> 6;
  std::uint64_t x = st[w] & (~std::uint64_t{0} << (j & 63));
  const std::size_t wl = (static_cast<std::size_t>(limit) + 63) >> 6;
  while (!x) {
    if (++w >= wl) return -1;
    x = st[w];
  }
  const int r = static_cast<int>((w << 6) + static_cast<std::size_t>(__builtin_ctzll(x)));
  return r < limit ? r : -1;
}

struct Cand {
  int pred_first;  // predecessor start (rule start + 2)
  int pred_len;    // in genes
  int rule = -1;   // index into rules, -1 unread
  int next_kept = -1;
  bool tried = false;
  bool kept = false;
  bool checked = false;
};

struct Scratch {
  Packed pk;
  std::vector<Cand> cand;
  std::vector<int> order, bucket_start;
  std::vector<Rule> rules;
};

}  // namespace

const std::vector<GeneSpan>& axiom_characters() {
  // Node-list order of the axiom network (output, hidden, input): see create_axiom_net.
  static const std::vector<GeneSpan> chars = {kAxiomOutput.span(), kAxiomHidden.span(),
                                              kAxiomInput.span()};
  return chars;
}

std::vector<Rule> generate_rules(const Genotype& g, const std::vector<GeneSpan>& node_chars) {
  thread_local Scratch sc;
  std::vector<Rule> result;
  const int n = static_cast<int>(g.size());
  const int last = n - 1;
  if (last - 0 <= 9) return result;
#if defined(__x86_64__)
  if (use_avx2())
    pack_avx2(g, sc.pk);
  else
#endif
    pack_scalar(g, sc.pk);
  const auto& B = sc.pk.b;
  auto& cand = sc.cand;
  cand.clear();
  sc.rules.clear();
  // Rule starts i in [0, last-10] with g[i] && g[i+1].
  const int imax = last - 10;
  const std::size_t nw = (static_cast<std::size_t>(imax) >> 6) + 1;
  for (std::size_t w = 0; w < nw; ++w) {
    std::uint64_t x = B[w] & ((B[w] >> 1) | (B[w + 1] << 63));
    while (x) {
      const int i = static_cast<int>((w << 6) + static_cast<std::size_t>(__builtin_ctzll(x)));
      x &= x - 1;
      if (i > imax) break;
      const int p = i + 2;
      const int par = p & 1;
      const auto& st = par ? sc.pk.s1 : sc.pk.s0;
      // Same-parity positions >= p with gene 1 and position < last.
      const int jlimit = (last - par + 1) >> 1;  // stream indices j with 2j+par < last
      const int jj = next_one(st, p >> 1, jlimit);
      int cur;
      if (jj >= 0) {
        cur = 2 * jj + par;
      } else {
        cur = p >= last ? p : p + 2 * ((last - p + 1) >> 1);
      }
      const int pred_last = cur - 1;
      if (pred_last + 9 <= last) cand.push_back(Cand{p, cur - p});
    }
  }
  if (cand.empty()) return result;

  // Bucket candidates by predecessor length (stable: genotype order within a length).
  int max_len = 0;
  for (const Cand& c : cand) max_len = std::max(max_len, c.pred_len);
  auto& bs = sc.bucket_start;
  bs.assign(max_len + 2, 0);
  for (const Cand& c : cand) ++bs[c.pred_len + 1];
  for (int l = 0; l <= max_len; ++l) bs[l + 1] += bs[l];
  auto& order = sc.order;
  order.resize(cand.size());
  {
    std::vector<int> fill(bs.begin(), bs.end() - 1);
    for (int i = 0; i < static_cast<int>(cand.size()); ++i) order[fill[cand[i].pred_len]++] = i;
  }

  int first_kept = -1;
  auto rule_of = [&](int idx) -> const Rule& { return sc.rules[cand[idx].rule]; };
  auto overlaps_kept = [&](int idx) {
    const Rule& r = rule_of(idx);
    for (int c = first_kept; c != -1; c = cand[c].next_kept) {
      if (c == idx) continue;
      const Rule& o = rule_of(c);
      if (o.gene_string.first <= r.gene_string.last && o.gene_string.last >= r.gene_string.first)
        return true;
    }
    return false;
  };
  auto pred_is_prefix = [&](const Cand& c, const GeneSpan& ch) {
    if (c.pred_len > ch.length()) return false;
    for (int k = 0; k < c.pred_len; ++k)
      if (g[c.pred_first + k] != ch.at(k)) return false;
    return true;
  };
  auto find_best = [&](const GeneSpan& ch) -> int {
    int len = std::min(ch.length(), max_len);
    for (; len >= 0; --len) {
      for (int o = bs[len]; o < bs[len + 1]; ++o) {
        const int idx = order[o];
        Cand& c = cand[idx];
        if (!pred_is_prefix(c, ch)) continue;
        if (c.rule < 0 && !c.tried) {
          c.tried = true;
          ReadRule rr = try_to_read_rule(g, c.pred_first - 2);
          if (rr.ok) {
            c.rule = static_cast<int>(sc.rules.size());
            sc.rules.push_back(rr.rule);
          }
        }
        if (c.rule >= 0 && !overlaps_kept(idx)) return idx;
      }
    }
    return -1;
  };
  auto insert_kept = [&](int idx) {
    const int start = cand[idx].pred_first;
    if (first_kept != -1 && cand[first_kept].pred_first < start) {
      int prev = first_kept;
      while (cand[prev].next_kept != -1 && cand[cand[prev].next_kept].pred_first < start)
        prev = cand[prev].next_kept;
      cand[idx].next_kept = cand[prev].next_kept;
      cand[prev].next_kept = idx;
    } else {
      cand[idx].next_kept = first_kept;
      first_kept = idx;
    }
  };

  int num_kept = 0;
  for (const GeneSpan& ch : node_chars) {
    const int best = find_best(ch);
    if (best != -1 && !cand[best].kept) {
      cand[best].kept = true;
      ++num_kept;
      insert_kept(best);
    }
  }
  int previous = 0;
  while (num_kept != previous) {
    previous = num_kept;
    for (int k = first_kept; k != -1; k = cand[k].next_kept) {
      if (cand[k].checked) continue;
      cand[k].checked = true;
      for (int s = 0; s < 2; ++s) {
        const GeneSpan ch = s ? rule_of(k).successor_new : rule_of(k).successor_replacement;
        const int keep = find_best(ch);
        if (keep != -1 && !cand[keep].kept) {
          cand[keep].kept = true;
          ++num_kept;
          insert_kept(keep);
        }
      }
    }
  }
  // Kept rules in genotype order of their predecessors (= candidate order).
  for (const Cand& c : cand)
    if (c.rule >= 0 && c.kept) result.push_back(sc.rules[c.rule]);
  return result;
}

Ann::Ann(Genotype genotype, int max_num_nodes)
    : genotype_(std::make_unique<Genotype>(std::move(genotype))), max_num_nodes_(max_num_nodes) {
  create_axiom_net();
  rules_ = generate_rules(*genotype_, axiom_characters());
}

void Ann::add_link(Node* from, Node* to, bool excitatory) {
  if (excitatory) {
    from->excitatory_outputs.push_back(to);
    to->excitatory_inputs.push_back(from);
  } else {
    from->inhibitory_outputs.push_back(to);
    to->inhibitory_inputs.push_back(from);
  }
}

void Ann::create_axiom_net() {
  // The original inserts each new node directly after the list head, so the
  // resulting order is output, hidden, input.
  nodes_.push_back(Node{});
  Node* o = &nodes_.back();
  o->character = kAxiomOutput.span();
  o->packed = pack_character(o->character);
  nodes_.push_back(Node{});
  Node* h = &nodes_.back();
  h->character = kAxiomHidden.span();
  h->packed = pack_character(h->character);
  nodes_.push_back(Node{});
  Node* i = &nodes_.back();
  i->character = kAxiomInput.span();
  i->packed = pack_character(i->character);
  add_link(i, h, true);
  add_link(h, o, true);
  net_inputs_.push_back(IoEntry{i, 0});
  i->net_input = &net_inputs_.back();
  net_outputs_.push_back(IoEntry{o, 0});
  o->net_output = &net_outputs_.back();
  num_nodes_ = 3;
}

Rule* Ann::pick_update_rule(const Node& node) {
  Rule* best = nullptr;
  int best_len = -999;
  for (Rule& r : rules_) {
    const int len = r.predecessor.length();
    if (len < best_len) continue;
    if (!r.predecessor.is_start_of(node.character)) continue;
    if (len > best_len) {
      best = &r;
      best_len = len;
    }
  }
  return best;
}

void Ann::morphogenesis_step() {
  if (!morphogenesis_steps_left_) return;
  for (Node& n : nodes_) n.just_constructed = false;
  for (auto it = nodes_.begin(); it != nodes_.end();) {
    auto next = std::next(it);
    if (!it->just_constructed) {
      Rule* r = pick_update_rule(*it);
      if (r) run_rule(*r, it);
      if (num_nodes_ == max_num_nodes_) break;
    }
    it = next;
  }
  if (num_nodes_ == max_num_nodes_)
    morphogenesis_steps_left_ = 0;
  else
    --morphogenesis_steps_left_;
}

namespace {
void erase_one(std::vector<Node*>& v, Node* n) {
  auto it = std::find(v.begin(), v.end(), n);
  if (it != v.end()) v.erase(it);
}
}  // namespace

void Ann::delete_node(std::list<Node>::iterator node_it) {
  Node* n = &*node_it;
  for (Node* x : n->inhibitory_inputs) erase_one(x->inhibitory_outputs, n);
  for (Node* x : n->excitatory_inputs) erase_one(x->excitatory_outputs, n);
  for (Node* y : n->inhibitory_outputs) erase_one(y->inhibitory_inputs, n);
  for (Node* y : n->excitatory_outputs) erase_one(y->excitatory_inputs, n);
  nodes_.erase(node_it);
}

void Ann::run_rule(Rule& rule, std::list<Node>::iterator node_it) {
  Node* node = &*node_it;
  Node* newnode = nullptr;
  if (!rule.successor_new.null()) {
    newnode = &*nodes_.insert(std::next(node_it), Node{});
    ++num_nodes_;
    newnode->character = rule.successor_new;
    newnode->packed = pack_character(newnode->character);
    const bool has_repl = !rule.successor_replacement.null();
    if (!has_repl ||
        (!rule.inhibitory_replacement_to_new && !rule.excitatory_replacement_to_new)) {
      const std::vector<Node*> inh = node->inhibitory_inputs;
      const std::vector<Node*> exc = node->excitatory_inputs;
      const bool rev = rule.reverse_inputs_type_on_new;
      for (Node* x : inh) add_link(x, newnode, rev);
      for (Node* x : exc) add_link(x, newnode, !rev);
    }
    if (!has_repl ||
        (!rule.inhibitory_new_to_replacement && !rule.excitatory_new_to_replacement)) {
      const std::vector<Node*> inh = node->inhibitory_outputs;
      const std::vector<Node*> exc = node->excitatory_outputs;
      const bool rev = rule.reverse_outputs_type_on_new;
      for (Node* y : inh) add_link(newnode, y, rev);
      for (Node* y : exc) add_link(newnode, y, !rev);
    }
  }
  const bool was_input = node->net_input != nullptr;
  const bool was_output = node->net_output != nullptr;
  if (!rule.successor_replacement.null()) {
    node->character = rule.successor_replacement;
    node->packed = pack_character(node->character);
    if (newnode) {
      if (rule.inhibitory_replacement_to_new) add_link(node, newnode, false);
      if (rule.excitatory_replacement_to_new) add_link(node, newnode, true);
      if (rule.inhibitory_new_to_replacement) add_link(newnode, node, false);
      if (rule.excitatory_new_to_replacement) add_link(newnode, node, true);
    }
  } else {
    if (node->net_input) {
      net_inputs_.remove_if([&](const IoEntry& e) { return &e == node->net_input; });
      node->net_input = nullptr;
    }
    if (node->net_output) {
      net_outputs_.remove_if([&](const IoEntry& e) { return &e == node->net_output; });
      node->net_output = nullptr;
    }
    delete_node(node_it);
    --num_nodes_;
  }
  if (newnode) {
    if (was_input) {
      net_inputs_.push_back(IoEntry{newnode, 0});
      newnode->net_input = &net_inputs_.back();
    }
    if (was_output) {
      net_outputs_.push_back(IoEntry{newnode, 0});
      newnode->net_output = &net_outputs_.back();
    }
  }
  rule.used = true;
}

void Ann::calc_new_outputs(Node& node, Ran1& rng, const NetConstants& k) {
  bool inhibited = false;
  for (const Node* x : node.inhibitory_inputs)
    if (x->inhibitory_output) {
      inhibited = true;
      break;
    }
  int e = 0;
  for (const Node* x : node.excitatory_inputs) e += x->excitatory_output;
  if (node.net_input) e += node.net_input->io_value;
  node.new_inhibitory_output = (e < k.t_v) ? 0 : 1;
  if (inhibited) {
    node.new_excitatory_output = 0;
  } else {
    if (rng.random(2)) {
      e += rng.random(k.max_noise);
    } else {
      e -= rng.random(k.max_noise);
      if (e < 0) e = 0;
    }
    int out = e >> 1;
    if (out < 0)
      out = 0;
    else if (out > kExcitatoryScale)
      out = kExcitatoryScale;
    node.new_excitatory_output = out;
  }
}

void Ann::update_outputs(Ran1& rng, const NetConstants& k) {
  for (Node& n : nodes_) calc_new_outputs(n, rng, k);
  for (Node& n : nodes_) {
    n.inhibitory_output = n.new_inhibitory_output;
    n.excitatory_output = n.new_excitatory_output;
    if (n.net_output) n.net_output->io_value = n.excitatory_output;
  }
  out_cache_.clear();
  for (const IoEntry& e : net_outputs_) out_cache_.push_back(OutCache{e.node->packed, e.io_value, e.node});
}

}  // namespace oee::geb

namespace oee::geb {

namespace {
// Character spans point into the genotype or into one of the axiom constants.
void put_span(Writer& w, const GeneSpan& c, const Genotype& g) {
  std::int32_t kind = 0;
  if (c.base == nullptr) kind = 0;
  else if (c.base == g.data()) kind = 1;
  else if (c.base == kAxiomOutput.genes.data()) kind = 2;
  else if (c.base == kAxiomHidden.genes.data()) kind = 3;
  else if (c.base == kAxiomInput.genes.data()) kind = 4;
  else throw std::runtime_error("unknown character base");
  w.put(kind); w.put<std::int32_t>(c.first); w.put<std::int32_t>(c.last);
}
GeneSpan get_span(Reader& r, const Genotype& g) {
  const auto kind = r.get<std::int32_t>();
  GeneSpan c;
  c.first = r.get<std::int32_t>();
  c.last = r.get<std::int32_t>();
  switch (kind) {
    case 0: c = GeneSpan{}; break;
    case 1: c.base = g.data(); break;
    case 2: c.base = kAxiomOutput.genes.data(); break;
    case 3: c.base = kAxiomHidden.genes.data(); break;
    case 4: c.base = kAxiomInput.genes.data(); break;
    default: throw std::runtime_error("bad character kind");
  }
  return c;
}
}  // namespace

void Ann::save_state(Writer& w) const {
  std::vector<std::uint8_t> used;
  for (const Rule& r : rules_) used.push_back(r.used ? 1 : 0);
  w.put_vec(used);
  std::vector<const Node*> order;
  for (const Node& n : nodes_) order.push_back(&n);
  auto index = [&](const Node* n) {
    for (std::size_t i = 0; i < order.size(); ++i)
      if (order[i] == n) return static_cast<std::int32_t>(i);
    throw std::runtime_error("dangling node");
  };
  w.put<std::uint64_t>(order.size());
  for (const Node* n : order) {
    put_span(w, n->character, *genotype_);
    w.put(n->inhibitory_output); w.put(n->excitatory_output);
    w.put(n->new_inhibitory_output); w.put(n->new_excitatory_output);
    w.put<std::uint8_t>(n->just_constructed ? 1 : 0);
    for (const auto* links : {&n->inhibitory_inputs, &n->excitatory_inputs,
                              &n->inhibitory_outputs, &n->excitatory_outputs}) {
      std::vector<std::int32_t> idx;
      for (const Node* x : *links) idx.push_back(index(x));
      w.put_vec(idx);
    }
  }
  for (const auto* io : {&net_inputs_, &net_outputs_}) {
    w.put<std::uint64_t>(io->size());
    for (const IoEntry& e : *io) { w.put(index(e.node)); w.put(e.io_value); }
  }
  w.put(num_nodes_); w.put(max_num_nodes_); w.put(morphogenesis_steps_left_);
}

void Ann::load_state(Reader& r) {
  const auto used = r.get_vec<std::uint8_t>();
  if (used.size() != rules_.size()) throw std::runtime_error("rule count mismatch on load");
  for (std::size_t i = 0; i < used.size(); ++i) rules_[i].used = used[i] != 0;
  nodes_.clear();
  net_inputs_.clear();
  net_outputs_.clear();
  const auto n = r.get<std::uint64_t>();
  std::vector<Node*> order;
  struct Pending { std::vector<std::int32_t> l[4]; };
  std::vector<Pending> pending(n);
  for (std::uint64_t i = 0; i < n; ++i) {
    nodes_.push_back(Node{});
    Node& nd = nodes_.back();
    order.push_back(&nd);
    nd.character = get_span(r, *genotype_);
    nd.packed = pack_character(nd.character);
    nd.inhibitory_output = r.get<int>(); nd.excitatory_output = r.get<int>();
    nd.new_inhibitory_output = r.get<int>(); nd.new_excitatory_output = r.get<int>();
    nd.just_constructed = r.get<std::uint8_t>() != 0;
    for (auto& l : pending[i].l) l = r.get_vec<std::int32_t>();
  }
  for (std::uint64_t i = 0; i < n; ++i) {
    Node& nd = *order[i];
    for (auto x : pending[i].l[0]) nd.inhibitory_inputs.push_back(order.at(x));
    for (auto x : pending[i].l[1]) nd.excitatory_inputs.push_back(order.at(x));
    for (auto x : pending[i].l[2]) nd.inhibitory_outputs.push_back(order.at(x));
    for (auto x : pending[i].l[3]) nd.excitatory_outputs.push_back(order.at(x));
  }
  for (int which = 0; which < 2; ++which) {
    auto& io = which ? net_outputs_ : net_inputs_;
    const auto m = r.get<std::uint64_t>();
    for (std::uint64_t i = 0; i < m; ++i) {
      Node* nd = order.at(r.get<std::int32_t>());
      const int v = r.get<int>();
      io.push_back(IoEntry{nd, v});
      (which ? nd->net_output : nd->net_input) = &io.back();
    }
  }
  num_nodes_ = r.get<int>(); max_num_nodes_ = r.get<int>(); morphogenesis_steps_left_ = r.get<int>();
  out_cache_.clear();
  for (const IoEntry& e : net_outputs_) out_cache_.push_back(OutCache{e.node->packed, e.io_value, e.node});
}

}  // namespace oee::geb
