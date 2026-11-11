#include "geb/world.hpp"

#include <algorithm>

#include "bff/soup.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>
#include <unordered_set>
#include <unordered_map>

namespace oee::geb {

namespace {
const Character kReproduce("01");
const Character kKill("100");
const Character kTurnAnticlockwise("101");
const Character kTurnClockwise("110");
const Character kMoveForward("111");
const Character kBond("000");    // L3 only
const Character kUnbond("001");  // L3 only
constexpr int kPrimingLevel = 2;
}  // namespace

// ---------------------------------------------------------------------------
// Organism

Organism::Organism(World& w, Genotype genotype, int x64, int y64)
    : Ann(std::move(genotype), w.p_.max_num_nodes), id(w.next_id_++), w_(w) {
  w.byid_[id] = this;
  colony = id;
  birth_time = w.ga_time_;
  at_x_64 = x64;
  at_y_64 = y64;
  at_x = at_x_64 >> 6;
  at_y = at_y_64 >> 6;
  at = at_x + at_y * w.length_;
  direction_deg = w.rng_.random(360);
  copy_net_inputs_from_neighbourhood();
  for (int rep = 0; rep < kPrimingLevel; ++rep) update_outputs(w.rng_, w.net_);
}

Organism::Organism(World& w, Genotype genotype, LoadTag)
    : Ann(std::move(genotype), w.p_.max_num_nodes), id(0), w_(w) {}

Organism::~Organism() {
  auto it = w_.byid_.find(id);
  if (it != w_.byid_.end() && it->second == this) w_.byid_.erase(it);
}

void Organism::copy_net_inputs_from_neighbourhood() {
  const int L = w_.length_;
  const int c = w_.p_.copy_to_length;
  struct In {
    IoEntry* entry;
    Gene side;
    bool packed;
    std::uint64_t tbits;
    int tn;
    GeneSpan test;
  };
  In ins[64];
  int n_in = 0, n_side[2] = {0, 0};
  std::vector<In> overflow;
  for (IoEntry& in : net_inputs()) {
    in.io_value = 0;
    const Node* inode = in.node;
    In x{&in, inode->character.at(1), inode->packed.packed, inode->packed.bits >> 1,
         inode->packed.nbits - 1,
         GeneSpan{inode->character.base, inode->character.first + 2, inode->character.last}};
    ++n_side[x.side];
    if (n_in < 64)
      ins[n_in++] = x;
    else
      overflow.push_back(x);
  }
  if (n_in == 0) return;
  // wrap_[v + c] == (v + L) % L for v in [-c, L - 1 + c] (precomputed).
  const int* wrap = w_.wrap_.data() + c;
  for (int cx = at_x - c; cx <= at_x + c; ++cx)
    for (int cy = at_y - c; cy <= at_y + c; ++cy) {
      const int wx = wrap[cx];
      const int wy = wrap[cy];
      if (wx == at_x && wy == at_y) continue;
      const Organism* other = w_.grid_[wx + wy * L].get();
      if (!other) continue;
      long dx = other->at_x_64 - at_x_64;
      long dy = other->at_y_64 - at_y_64;
      if (dx > (L << (6 - 1)))
        dx -= L << 6;
      else if (dx < -(L << (6 - 1)))
        dx += L << 6;
      if (dy > (L << (6 - 1)))
        dy -= L << 6;
      else if (dy < -(L << (6 - 1)))
        dy += L << 6;
      const int delta_right = static_cast<int>(dx * w_.cos64_[direction_deg] +
                                               dy * w_.sin64_[direction_deg]);
      const Gene right = (delta_right > 0) ? 1 : 0;
      if (!n_side[right]) continue;
      // Contributions are (scale*io)>>6; zero-io outputs add nothing, so the distance
      // and scale are computed only when a non-zero contribution is possible.
      int scale = -1;
      auto contribute = [&](const In& x) {
        for (const Ann::OutCache& out : other->output_cache()) {
          if (!out.io_value) continue;
          const PackedChar& oc = out.packed;
          const bool match = (x.packed && oc.packed) ? packed_is_prefix(x.tbits, x.tn, oc)
                                                     : x.test.is_start_of(out.node->character);
          if (!match) continue;
          if (scale < 0) {
            const long d = static_cast<long>(std::sqrt(static_cast<double>(dx * dx + dy * dy)));
            scale = static_cast<int>(World::kCopyScale64AtDistance1 / d);
          }
          x.entry->io_value += ((scale * out.io_value) >> 6);
        }
      };
      for (int k = 0; k < n_in; ++k)
        if (ins[k].side == right) contribute(ins[k]);
      for (const In& x : overflow)
        if (x.side == right) contribute(x);
    }
}

int Organism::action_output(const GeneSpan& prefix) {
  long sum = -1;
  for (const Ann::OutCache& out : output_cache())
    if (prefix.is_start_of(out.node->character)) {
      if (sum == -1) sum = 0;
      sum += out.io_value;
      if (sum > kExcitatoryScale) {
        sum = kExcitatoryScale;
        break;
      }
    }
  if (sum == -1) sum = w_.rng_.random(World::kMaxNoiseOnIsolatedOutputs);
  return static_cast<int>(sum);
}

void Organism::actions() {
  pen_colour = -1;
  long o = action_output(kReproduce.span());
  if (o > World::kTA) {
    pen_colour = 1;
    try_to_reproduce_with_organism_in_front();
  }
  o = action_output(kKill.span());
  if (o > World::kTA) {
    pen_colour = 2;
    try_to_kill_organism_in_front();
  }
  o = action_output(kTurnAnticlockwise.span());
  turn_anti_clockwise(static_cast<int>((World::kMaxTurnDeg * o) >> kExcitatoryScaleShift));
  o = action_output(kTurnClockwise.span());
  turn_clockwise(static_cast<int>((World::kMaxTurnDeg * o) >> kExcitatoryScaleShift));
  o = action_output(kMoveForward.span());
  try_to_move_forward(static_cast<int>((o << 6) >> kExcitatoryScaleShift));
  if (w_.p_.collectives == 2) {
    // L3 control: grouping decisions are coin flips, not network outputs, so grouping
    // tendency has no heritable variation (bond p = 1/2, unbond p = 1/20).
    w_.prune_bonds(this);
    bond_out = w_.rng_.random(2) == 1;
    if (bond_out) try_to_bond_with_organism_in_front();
    if (w_.rng_.random(20) == 0) unbond_all();
  } else if (w_.p_.collectives) {
    w_.prune_bonds(this);
    o = action_output(kBond.span());
    bond_out = o > World::kTA;
    if (bond_out) try_to_bond_with_organism_in_front();
    o = action_output(kUnbond.span());
    if (o > World::kTA) unbond_all();
  }
}

void Organism::try_to_bond_with_organism_in_front() {
  World& w = w_;
  Organism* other = w.grid_[w.forward1(at_x_64, at_y_64, direction_deg)].get();
  if (!other || other == this || !w.in_bond_range(this, other)) return;
  if (other->colony != colony) return;  // kin bonds only
  World::bond(this, other);
}

void Organism::unbond_all() {
  for (std::uint64_t id : bonds)
    if (Organism* p = w_.find_by_id(id))
      p->bonds.erase(std::remove(p->bonds.begin(), p->bonds.end(), this->id), p->bonds.end());
  bonds.clear();
}

Organism* World::find_by_id(std::uint64_t id) const {
  auto it = byid_.find(id);
  return it == byid_.end() ? nullptr : it->second;
}

bool World::in_bond_range(const Organism* a, const Organism* b) const {
  auto d = [this](int u, int v) { int x = std::abs(u - v); return std::min(x, length_ - x); };
  return std::max(d(a->at_x, b->at_x), d(a->at_y, b->at_y)) <= kBondRange;
}

void World::prune_bonds(Organism* o) const {
  // Drop bonds to dead agents and to agents out of range (bonds are physical).
  std::vector<std::uint64_t> keep;
  for (std::uint64_t id : o->bonds) {
    Organism* p = find_by_id(id);
    if (p && in_bond_range(o, p)) {
      keep.push_back(id);
    } else if (p) {
      p->bonds.erase(std::remove(p->bonds.begin(), p->bonds.end(), o->id), p->bonds.end());
    }
  }
  o->bonds.swap(keep);
}

void World::bond(Organism* a, Organism* b) {
  if (std::find(a->bonds.begin(), a->bonds.end(), b->id) != a->bonds.end()) return;
  a->w_.prune_bonds(a);
  a->w_.prune_bonds(b);
  if (static_cast<int>(a->bonds.size()) >= kMaxBonds || static_cast<int>(b->bonds.size()) >= kMaxBonds) return;
  a->bonds.push_back(b->id);
  b->bonds.push_back(a->id);
}

int World::group_size(const Organism* o, std::vector<const Organism*>* members) const {
  const auto& byid = byid_;
  std::vector<const Organism*> stack{o}, seen{o};
  std::unordered_set<std::uint64_t> visited{o->id};
  while (!stack.empty()) {
    const Organism* x = stack.back();
    stack.pop_back();
    for (std::uint64_t id : x->bonds) {
      auto it = byid.find(id);
      if (it == byid.end() || !in_bond_range(x, it->second) || !visited.insert(id).second) continue;
      stack.push_back(it->second);
      seen.push_back(it->second);
    }
  }
  if (members) *members = seen;
  return static_cast<int>(seen.size());
}

World::GroupStats World::group_stats() const {
  GroupStats g;
  const auto& byid = byid_;
  std::unordered_set<std::uint64_t> done;
  int agents = 0, bonded = 0;
  for (int c = 0; c < size_; ++c) {
    const Organism* o = grid_[c].get();
    if (!o) continue;
    ++agents;
    bool live = false;
    for (auto id : o->bonds) {
      auto it = byid.find(id);
      live |= it != byid.end() && in_bond_range(o, it->second);
    }
    bonded += live;
    if (done.count(o->id)) continue;
    std::vector<const Organism*> m;
    const int n = group_size(o, &m);
    for (auto* x : m) done.insert(x->id);
    ++g.groups;
    g.largest = std::max(g.largest, n);
  }
  g.mean = g.groups ? double(agents) / g.groups : 0;
  g.bonded = agents ? double(bonded) / agents : 0;
  return g;
}

void Organism::try_to_reproduce_with_organism_in_front() {
  World& w = w_;
  const int other_at = w.forward1(at_x_64, at_y_64, direction_deg);
  Organism* other = w.grid_[other_at].get();
  if (!other) return;
  int child_at = w.forward1(other->at_x_64, other->at_y_64, direction_deg);
  int cx64, cy64;
  bool replace = false;
  if (w.grid_[child_at] || w.num_individuals_ == w.p_.max_num_individuals) {
    child_at = other_at;
    if (other->pen_colour == 2) return;  // disallow_reproduction_into_killer
    cx64 = other->at_x_64;
    cy64 = other->at_y_64;
    replace = true;
  } else {
    ++w.num_individuals_;
    cx64 = ((child_at % w.length_) << 6) + 32;
    cy64 = ((child_at / w.length_) << 6) + 32;
  }
  Organism* pa = this;
  Organism* pb = other;
  if (w.p_.random_selection) {
    // GebR control (GPEM sec. 10): genetic parents are random real agents, and a
    // replacement birth kills a random real agent (other than the actor).
    const int a = w.random_occupied_cell(-1);
    int b = a;
    while (b == a) b = w.random_occupied_cell(-1);
    pa = w.grid_[a].get();
    pb = w.grid_[b].get();
    if (replace) {
      child_at = w.random_occupied_cell(at);
      cx64 = w.grid_[child_at]->at_x_64;
      cy64 = w.grid_[child_at]->at_y_64;
    }
  }
  Genotype shadow_child;
  std::unique_ptr<Organism> child = w.reproduce(pa, pb, cx64, cy64, shadow_child);
  if (!child) {  // M5 v3: child without a working copier is not born
    if (!replace) --w.num_individuals_;
    return;
  }
  if (w.p_.collectives && bond_out && w.group_size(this) < World::kColonyMax) {  // L3 budding
    child->colony = colony;
    World::bond(this, child.get());
  }
  if (replace) {
    std::unique_ptr<Organism> old = std::move(w.grid_[child_at]);
    w.grid_[child_at] = std::move(child);
    old.reset();
    w.kill_random_shadow();
  } else {
    w.grid_[child_at] = std::move(child);
  }
  w.add_shadow(std::move(shadow_child));
}

void Organism::try_to_kill_organism_in_front() {
  World& w = w_;
  if (w.num_individuals_ == w.p_.min_num_individuals) return;
  int other_at = w.forward1(at_x_64, at_y_64, direction_deg);
  if (w.grid_[other_at] && w.p_.collectives) {
    // L3: no killing within a group; an attack succeeds only if the attacker's group is
    // at least as large as the victim's.
    std::vector<const Organism*> mine;
    const int na = w.group_size(this, &mine);
    const Organism* victim = w.grid_[other_at].get();
    if (std::find(mine.begin(), mine.end(), victim) != mine.end()) return;
    if (na < w.group_size(victim)) return;
  }
  if (w.grid_[other_at]) {
    if (w.p_.random_selection) other_at = w.random_occupied_cell(at);
    w.remove_real(other_at).reset();
    --w.num_individuals_;
  }
}

void Organism::turn_anti_clockwise(int by_deg) {
  if (!by_deg) return;
  direction_deg = (direction_deg + 360 - (by_deg % 360)) % 360;
}

void Organism::turn_clockwise(int by_deg) {
  if (!by_deg) return;
  direction_deg = (direction_deg + by_deg) % 360;
}

void Organism::try_to_move_forward(int distance_64) {
  if (!distance_64) return;
  World& w = w_;
  const int L64 = w.length_ << 6;
  int to_x_64 = at_x_64 + ((distance_64 * w.sin64_[direction_deg]) / (1 << 6));
  if (to_x_64 < 0)
    to_x_64 += L64;
  else if (to_x_64 >= L64)
    to_x_64 -= L64;
  int to_y_64 = at_y_64 - ((distance_64 * w.cos64_[direction_deg]) / (1 << 6));
  if (to_y_64 < 0)
    to_y_64 += L64;
  else if (to_y_64 >= L64)
    to_y_64 -= L64;
  const int to_x = to_x_64 >> 6, to_y = to_y_64 >> 6;
  const int to = to_x + to_y * w.length_;
  if (to == at || !w.grid_[to]) {
    if (to != at) {
      w.grid_[to] = std::move(w.grid_[at]);
      at = to;
    }
    at_x_64 = to_x_64;
    at_y_64 = to_y_64;
    at_x = to_x;
    at_y = to_y;
  }
}

// ---------------------------------------------------------------------------
// World

void World::init_tables() {
  for (int t = 0; t < 360; ++t) {
    cos64_[t] = static_cast<int>(64.0 * std::cos(0.017453293 * t));
    sin64_[t] = static_cast<int>(64.0 * std::sin(0.017453293 * t));
  }
  length_ = p_.world_length;
  size_ = length_ * length_;
  grid_.resize(size_);
  np_positions_.resize(size_);
  np_ids_.resize(size_);
  shadow_genotypes_.reserve(size_);
}

World::World(const Params& p, Empty) : p_(p) {
  init_tables();
  init_wrap();  // p_ already holds the adjusted copy_to_length when loaded
}

void World::init_wrap() {
  const int c = p_.copy_to_length;
  wrap_.resize(length_ + 2 * c);
  for (int v = -c; v < length_ + c; ++v) wrap_[v + c] = (v + length_) % length_;
}

World::World(const Params& p) : p_(p) {
  seed_used_ = rng_.seed(p_.seed);
  if (p_.second_shadow) rng2_.seed(seed_used_ % 30000 + 1777);
  init_tables();
  if (p_.min_num_individuals == -1) p_.min_num_individuals = 0;
  if (p_.max_num_individuals == -1 || p_.max_num_individuals > size_)
    p_.max_num_individuals = size_;
  if (p_.start_num_individuals == -1 || p_.start_num_individuals > p_.max_num_individuals)
    p_.start_num_individuals = p_.max_num_individuals;
  if (2 * p_.copy_to_length + 1 > length_) p_.copy_to_length = (length_ - 1) / 2;
  init_wrap();
  ga_time_ = -1;
  for (int at = 0; at < p_.start_num_individuals; ++at) {
    grid_[at] = std::make_unique<Organism>(*this, Genotype{0}, ((at % length_) << 6) + 32,
                                           ((at / length_) << 6) + 32);
    add_shadow(Genotype{0});
  }
  num_individuals_ = p_.start_num_individuals;
  reset_not_processed();
  num_reproductions_ = 0;
}

World::~World() = default;

int World::forward1(int from_x_64, int from_y_64, int dir) const {
  int x_64 = from_x_64, y_64 = from_y_64;
  const int dx = sin64_[dir];
  const int dy = -cos64_[dir];
  const int fx = from_x_64 >> 6, fy = from_y_64 >> 6;
  int x = fx, y = fy;
  const int L64 = length_ << 6;
  while (x == fx && y == fy) {
    x_64 += dx;
    y_64 += dy;
    if (x_64 < 0)
      x_64 += L64;
    else if (x_64 >= L64)
      x_64 -= L64;
    if (y_64 < 0)
      y_64 += L64;
    else if (y_64 >= L64)
      y_64 -= L64;
    x = x_64 >> 6;
    y = y_64 >> 6;
  }
  return x + y * length_;
}

int World::random_occupied_cell(int exclude) {
  occupied_.clear();
  for (int c = 0; c < size_; ++c)
    if (grid_[c] && c != exclude) occupied_.push_back(c);
  return occupied_[rng_.random(static_cast<int>(occupied_.size()))];
}

void World::kill_random_shadow() {
  const int del = rng_.random(shadow_count());
  const int last = shadow_count() - 1;
  shadow_genotypes_[del] = std::move(shadow_genotypes_[last]);
  shadow_ages_[del] = shadow_ages_[last];
  shadow_birth_times_[del] = shadow_birth_times_[last];
  shadow_genotypes_.pop_back();
  shadow_ages_.pop_back();
  shadow_birth_times_.pop_back();
  if (p_.second_shadow) {
    const int d2 = rng2_.random(static_cast<int>(shadow2_.size()));
    shadow2_[d2] = std::move(shadow2_.back());
    shadow2_.pop_back();
  }
}

std::unique_ptr<Organism> World::remove_real(int cell) {
  std::unique_ptr<Organism> o = std::move(grid_[cell]);
  kill_random_shadow();
  return o;
}

void World::add_shadow(Genotype g) {
  if (p_.second_shadow) {
    shadow2_.push_back(pending_shadow2_.empty() ? g : std::move(pending_shadow2_));
    pending_shadow2_.clear();
  }
  shadow_genotypes_.push_back(std::move(g));
  shadow_ages_.push_back(0);
  shadow_birth_times_.push_back(ga_time_);
}

namespace {
std::vector<std::uint8_t> to_bytes(const Genotype& g) {
  std::vector<std::uint8_t> b((g.size() + 7) / 8, 0);
  for (std::size_t i = 0; i < g.size(); ++i)
    if (g[i]) b[i / 8] |= static_cast<std::uint8_t>(0x80 >> (i % 8));
  return b;
}
}  // namespace

Genotype World::bff_breed(Ran1& r, const Genotype& g1, const Genotype& /*g2*/, int heredity) {
  // M5 v1 (docs/M5.md): asexual, fixed length. Tape = [g1 | random buffer of the same
  // length]; g1's code runs on it as BFF; the child is the buffer. The mate triggers the
  // birth (Geb's reproduce action) but contributes no genes. Background mutation: each
  // tape byte is replaced with probability 2^-12, as in the BFF soup.
  const std::vector<std::uint8_t> b1 = to_bytes(g1);
  const int l1 = static_cast<int>(b1.size());
  std::vector<std::uint8_t> tape(b1);
  // v1 (heredity 1): random buffer. v2 (heredity 2): zero buffer, so a child receives only
  // what the parent's code writes (no free genetic material for non-copiers). v3
  // (heredity 3): random buffer as v1, plus a viability rule applied by the caller.
  for (int i = 0; i < l1; ++i) tape.push_back(heredity == 2 ? 0 : static_cast<std::uint8_t>(r.random(256)));
  for (auto& byte : tape)
    if (r.random(4096) == 0) byte = static_cast<std::uint8_t>(r.random(256));
  const int n = static_cast<int>(tape.size());
  bff::evaluate_n(tape.data(), n, std::size_t(bff::kMaxSteps) * std::max(1, n / bff::kPair));
  Genotype child(std::size_t(l1) * 8);
  for (int i = 0; i < l1 * 8; ++i) child[i] = (tape[l1 + i / 8] >> (7 - i % 8)) & 1;
  return child;
}

bool World::bff_viable(Ran1& r, const Genotype& g) {
  // v3 viability: the genome's own code, run on [g | random buffer], passes on at least
  // kViableBytes informative (non-zero) bytes of g (best alignment within +-8).
  constexpr int kViableBytes = 16;
  const std::vector<std::uint8_t> b = to_bytes(g);
  const int l = static_cast<int>(b.size());
  std::vector<std::uint8_t> tape(b);
  for (int i = 0; i < l; ++i) tape.push_back(static_cast<std::uint8_t>(r.random(256)));
  const int n = static_cast<int>(tape.size());
  bff::evaluate_n(tape.data(), n, std::size_t(bff::kMaxSteps) * std::max(1, n / bff::kPair));
  for (int sh = -8; sh <= 8; ++sh) {
    int m = 0;
    for (int k = 0; k < l; ++k) {
      const int j = k + sh;
      if (j >= 0 && j < l) m += b[k] != 0 && b[k] == tape[l + j];
    }
    if (m >= kViableBytes) return true;
  }
  return false;
}

Genotype World::bff_breed_viable(Ran1& r, const std::vector<const Genotype*>& pool) {
  // Shadow populations under v3: redraw parents until the child is viable (at most 64
  // draws), so the null applies the same viability rule as the real population.
  for (int t = 0; t < 64; ++t) {
    const Genotype& g = *pool[r.random(static_cast<int>(pool.size()))];
    Genotype c = bff_breed(r, g, g, 3);
    if (bff_viable(r, c)) return c;
  }
  return *pool[r.random(static_cast<int>(pool.size()))];
}

void World::seed_genomes(const std::vector<Genotype>& pool) {
  if (pool.empty()) return;
  // Under BFF heredity, each initial agent is a child of a founder (one application of
  // the founder's own copy code), so uncopied bytes are randomised as at any later birth.
  auto pick = [&](std::size_t k, Ran1& r) {
    const Genotype& g = pool[k % pool.size()];
    return p_.heredity ? bff_breed(r, g, g, p_.heredity) : g;
  };
  std::size_t k = 0;
  for (int at = 0; at < size_; ++at)
    if (grid_[at]) {
      const int x = grid_[at]->at_x_64, y = grid_[at]->at_y_64;
      grid_[at] = std::make_unique<Organism>(*this, pick(k++, rng_), x, y);
    }
  for (std::size_t i = 0; i < shadow_genotypes_.size(); ++i) shadow_genotypes_[i] = pick(i, rng_);
  for (std::size_t i = 0; i < shadow2_.size(); ++i) shadow2_[i] = pick(i, rng2_);
  reset_not_processed();
}

Genotype World::breed_second_shadow() {
  Ran1& r = rng2_;
  const int n = static_cast<int>(shadow2_.size());
  int a = r.random(n);
  int b = a;
  while (b == a) b = r.random(n);
  if (r.random(2)) std::swap(a, b);
  const Genotype& p1 = shadow2_[a];
  const Genotype& p2 = shadow2_[b];
  if (p_.heredity == 3) {
    std::vector<const Genotype*> pool;
    for (const Genotype& g : shadow2_) pool.push_back(&g);
    return bff_breed_viable(r, pool);
  }
  if (p_.heredity) return bff_breed(r, p1, p2, p_.heredity);
  const int cp1 = r.random(static_cast<int>(p1.size()) + 1);
  int cp2 = cp1 + (r.random(2) ? -kGenotypeLengthChangeMax : kGenotypeLengthChangeMax);
  const int len2 = static_cast<int>(p2.size());
  if (cp2 < 0) cp2 = 0;
  if (cp2 >= len2) cp2 = len2 - 1;
  Genotype child(p1.begin(), p1.begin() + cp1);
  child.insert(child.end(), p2.begin() + cp2, p2.end());
  for (int m = 0; m < kNumMutations; ++m) {
    Gene& gene = child[r.random(static_cast<int>(child.size()))];
    gene = gene == 0 ? 1 : 0;
  }
  return child;
}

namespace {
Genotype breed(Ran1&, const Genotype& p1, const Genotype& p2, int cp1, int cp2) {
  const int len2 = static_cast<int>(p2.size());
  Genotype child;
  child.reserve(cp1 + len2 - cp2);
  child.insert(child.end(), p1.begin(), p1.begin() + cp1);
  child.insert(child.end(), p2.begin() + cp2, p2.end());
  return child;
}
}  // namespace

std::unique_ptr<Organism> World::reproduce(Organism* a, Organism* b, int cx64, int cy64,
                                           Genotype& shadow_child) {
  Organism* p1;
  Organism* p2;
  if (rng_.random(2) == 0) {
    p1 = a;
    p2 = b;
  } else {
    p1 = b;
    p2 = a;
  }
  const int n = shadow_count();
  const int s1 = rng_.random(n);
  int s2 = s1;
  while (s2 == s1) s2 = rng_.random(n);
  const Genotype& sp1 = shadow_genotypes_[s1];
  const Genotype& sp2 = shadow_genotypes_[s2];
  const Genotype& g1 = p1->genotype();
  const Genotype& g2 = p2->genotype();
  if (p_.heredity == 3) {
    Genotype child = bff_breed(rng_, g1, g2, 3);
    if (!bff_viable(rng_, child)) return nullptr;  // no working copier: no birth
    std::vector<const Genotype*> pool;
    for (const Genotype& g : shadow_genotypes_) pool.push_back(&g);
    shadow_child = bff_breed_viable(rng_, pool);
    if (p_.second_shadow) pending_shadow2_ = breed_second_shadow();
    auto o = std::make_unique<Organism>(*this, std::move(child), cx64, cy64);
    ++num_reproductions_;
    return o;
  }
  if (p_.heredity) {
    Genotype child = bff_breed(rng_, g1, g2, p_.heredity);
    shadow_child = bff_breed(rng_, sp1, sp2, p_.heredity);
    if (p_.second_shadow) pending_shadow2_ = breed_second_shadow();
    auto o = std::make_unique<Organism>(*this, std::move(child), cx64, cy64);
    ++num_reproductions_;
    return o;
  }

  int cp1 = rng_.random(static_cast<int>(g1.size()) + 1);
  int cp2 = cp1;
  int scp1 = rng_.random(static_cast<int>(sp1.size()) + 1);
  int scp2 = scp1;
  if (rng_.random(100) < kGenotypeLengthChangeProb) {
    if (rng_.random(2))
      cp2 -= kGenotypeLengthChangeMax;
    else
      cp2 += kGenotypeLengthChangeMax;
  }
  if (rng_.random(100) < kGenotypeLengthChangeProb) {
    if (rng_.random(2))
      scp2 -= kGenotypeLengthChangeMax;
    else
      scp2 += kGenotypeLengthChangeMax;
  }
  const int len2 = static_cast<int>(g2.size());
  const int slen2 = static_cast<int>(sp2.size());
  if (cp2 < 0) cp2 = 0;
  if (cp2 >= len2) cp2 = len2 - 1;
  if (scp2 < 0) scp2 = 0;
  if (scp2 >= slen2) scp2 = slen2 - 1;
  Genotype child = breed(rng_, g1, g2, cp1, cp2);
  shadow_child = breed(rng_, sp1, sp2, scp1, scp2);
  for (int m = 0; m < kNumMutations; ++m) {
    Gene& gene = child[rng_.random(static_cast<int>(child.size()))];
    gene = gene == 0 ? 1 : 0;
  }
  for (int m = 0; m < kNumMutations; ++m) {
    Gene& gene = shadow_child[rng_.random(static_cast<int>(shadow_child.size()))];
    gene = gene == 0 ? 1 : 0;
  }
  if (p_.second_shadow) pending_shadow2_ = breed_second_shadow();
  auto o = std::make_unique<Organism>(*this, std::move(child), cx64, cy64);
  ++num_reproductions_;
  return o;
}

void World::reset_not_processed() {
  ++ga_time_;
  num_not_processed_ = 0;
  for (int at = 0; at < size_; ++at)
    if (grid_[at]) {
      np_positions_[num_not_processed_] = at;
      np_ids_[num_not_processed_] = grid_[at]->id;
      ++num_not_processed_;
    }
}

void World::step() {
  if (num_not_processed_ == 0) return;
  const int r = rng_.random(num_not_processed_);
  Organism* ind = grid_[np_positions_[r]].get();
  if (ind && ind->id == np_ids_[r] && ind->birth_time != ga_time_) {
    ind->copy_net_inputs_from_neighbourhood();
    ind->morphogenesis_step();
    ind->update_outputs(rng_, net_);
    ind->actions();
    ++ind->age;
  }
  --num_not_processed_;
  if (num_not_processed_) {
    np_positions_[r] = np_positions_[num_not_processed_];
    np_ids_[r] = np_ids_[num_not_processed_];
  } else {
    for (int s = 0; s < shadow_count(); ++s)
      if (shadow_birth_times_[s] != ga_time_) ++shadow_ages_[s];
    reset_not_processed();
  }
}

void World::reseed(int s) {
  rng_.seed(s);
  if (p_.second_shadow) rng2_.seed(s % 30000 + 1777);
}

void World::reset_shadow_to_real() {
  int k = -1;
  for (int at = 0; at < size_; ++at) {
    const Organism* o = grid_[at].get();
    if (!o) continue;
    ++k;
    shadow_genotypes_[k] = o->genotype();
    shadow_ages_[k] = o->age;
    shadow_birth_times_[k] = o->birth_time;
    if (p_.second_shadow) shadow2_[k] = o->genotype();
  }
}

void World::terminate() {
  for (int d = 0; d < size_; ++d)
    if (grid_[d]) remove_real(d).reset();
}

}  // namespace oee::geb

namespace oee::geb {

namespace {
constexpr std::uint64_t kWorldMagic = 0x4f45453147454257ULL;  // "WBEG1EEO"
// Coupled (M5) worlds use their own magic so that Params::heredity, which lives in
// padding bytes of older checkpoints, is only trusted when it was written deliberately.
constexpr std::uint64_t kWorldMagicCoupled = 0x4f45453547454257ULL;
// Worlds with L3 collectives (and possibly M5 heredity) store Params::collectives and
// per-agent bond lists; padding bytes of older checkpoints are never trusted.
constexpr std::uint64_t kWorldMagicL3 = 0x4f45453347454257ULL;
}

void World::save(Writer& w) const {
  w.put(p_.collectives ? kWorldMagicL3 : p_.heredity ? kWorldMagicCoupled : kWorldMagic);
  w.put(p_);
  w.put(seed_used_);
  rng_.save(w);
  rng2_.save(w);
  w.put(ga_time_); w.put(num_reproductions_); w.put(num_individuals_); w.put(next_id_);
  for (int c = 0; c < size_; ++c) {
    const Organism* o = grid_[c].get();
    w.put<std::uint8_t>(o ? 1 : 0);
    if (!o) continue;
    w.put_vec(o->genotype());
    w.put(o->id); w.put(o->pen_colour); w.put(o->at); w.put(o->at_x); w.put(o->at_y);
    w.put(o->at_x_64); w.put(o->at_y_64); w.put(o->direction_deg); w.put(o->birth_time);
    w.put(o->age);
    o->save_state(w);
    if (p_.collectives) {
      w.put_vec(o->bonds);
      w.put<std::uint8_t>(o->bond_out);
      w.put(o->colony);
    }
  }
  w.put<std::uint64_t>(shadow_genotypes_.size());
  for (const Genotype& g : shadow_genotypes_) w.put_vec(g);
  w.put_vec(shadow_ages_);
  w.put_vec(shadow_birth_times_);
  w.put<std::uint64_t>(shadow2_.size());
  for (const Genotype& g : shadow2_) w.put_vec(g);
  w.put_vec(pending_shadow2_);
  w.put_vec(np_positions_);
  w.put_vec(np_ids_);
  w.put(num_not_processed_);
}

std::unique_ptr<World> World::load(Reader& r) {
  const auto magic = r.get<std::uint64_t>();
  if (magic != kWorldMagic && magic != kWorldMagicCoupled && magic != kWorldMagicL3)
    throw std::runtime_error("not a world checkpoint");
  Params p = r.get<Params>();
  if (magic == kWorldMagic) p.heredity = 0;
  if (magic != kWorldMagicL3) p.collectives = 0;
  std::unique_ptr<World> w(new World(p, Empty{}));
  w->seed_used_ = r.get<int>();
  w->rng_.load(r);
  w->rng2_.load(r);
  w->ga_time_ = r.get<long>(); w->num_reproductions_ = r.get<long>();
  w->num_individuals_ = r.get<int>(); w->next_id_ = r.get<std::uint64_t>();
  for (int c = 0; c < w->size_; ++c) {
    if (!r.get<std::uint8_t>()) continue;
    auto o = std::make_unique<Organism>(*w, r.get_vec<Gene>(), Organism::LoadTag{});
    o->id = r.get<std::uint64_t>(); o->pen_colour = r.get<int>(); o->at = r.get<int>();
    o->at_x = r.get<int>(); o->at_y = r.get<int>(); o->at_x_64 = r.get<int>();
    o->at_y_64 = r.get<int>(); o->direction_deg = r.get<int>(); o->birth_time = r.get<long>();
    o->age = r.get<int>();
    o->load_state(r);
    if (p.collectives) {
      o->bonds = r.get_vec<std::uint64_t>();
      o->bond_out = r.get<std::uint8_t>();
      o->colony = r.get<std::uint64_t>();
    }
    w->byid_[o->id] = o.get();
    w->grid_[c] = std::move(o);
  }
  const auto ns = r.get<std::uint64_t>();
  for (std::uint64_t k = 0; k < ns; ++k) w->shadow_genotypes_.push_back(r.get_vec<Gene>());
  w->shadow_ages_ = r.get_vec<int>();
  w->shadow_birth_times_ = r.get_vec<long>();
  const auto ns2 = r.get<std::uint64_t>();
  for (std::uint64_t k = 0; k < ns2; ++k) w->shadow2_.push_back(r.get_vec<Gene>());
  w->pending_shadow2_ = r.get_vec<Gene>();
  w->np_positions_ = r.get_vec<int>();
  w->np_ids_ = r.get_vec<std::uint64_t>();
  w->num_not_processed_ = r.get<int>();
  return w;
}

}  // namespace oee::geb
