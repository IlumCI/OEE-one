#include "geb/world.hpp"

#include <cmath>

namespace oee::geb {

namespace {
const Character kReproduce("01");
const Character kKill("100");
const Character kTurnAnticlockwise("101");
const Character kTurnClockwise("110");
const Character kMoveForward("111");
constexpr int kPrimingLevel = 2;
}  // namespace

// ---------------------------------------------------------------------------
// Organism

Organism::Organism(World& w, Genotype genotype, int x64, int y64)
    : Ann(std::move(genotype), w.p_.max_num_nodes), id(w.next_id_++), w_(w) {
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

void Organism::copy_net_inputs_from_neighbourhood() {
  const int L = w_.length_;
  const int c = w_.p_.copy_to_length;
  for (IoEntry& in : net_inputs()) in.io_value = 0;
  for (int cx = at_x - c; cx <= at_x + c; ++cx)
    for (int cy = at_y - c; cy <= at_y + c; ++cy) {
      const int wx = (cx + L) % L;
      const int wy = (cy + L) % L;
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
      const long d = static_cast<long>(std::sqrt(static_cast<double>(dx * dx + dy * dy)));
      const int scale = static_cast<int>(World::kCopyScale64AtDistance1 / d);
      const int delta_right = static_cast<int>(dx * w_.cos64_[direction_deg] +
                                               dy * w_.sin64_[direction_deg]);
      const Gene right = (delta_right > 0) ? 1 : 0;
      for (IoEntry& in : net_inputs()) {
        const GeneSpan& ch = in.node->character;
        if (ch.at(1) != right) continue;
        GeneSpan test = ch;
        test.first += 2;
        for (const IoEntry& out : other->net_outputs())
          if (test.is_start_of(out.node->character))
            in.io_value += ((scale * out.io_value) >> 6);
      }
    }
}

int Organism::action_output(const GeneSpan& prefix) {
  long sum = -1;
  for (const IoEntry& out : net_outputs())
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
  Genotype shadow_child;
  std::unique_ptr<Organism> child = w.reproduce(this, other, cx64, cy64, shadow_child);
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
  const int other_at = w.forward1(at_x_64, at_y_64, direction_deg);
  if (w.grid_[other_at]) {
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

World::World(const Params& p) : p_(p) {
  seed_used_ = rng_.seed(p_.seed);
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
  if (p_.min_num_individuals == -1) p_.min_num_individuals = 0;
  if (p_.max_num_individuals == -1 || p_.max_num_individuals > size_)
    p_.max_num_individuals = size_;
  if (p_.start_num_individuals == -1 || p_.start_num_individuals > p_.max_num_individuals)
    p_.start_num_individuals = p_.max_num_individuals;
  if (2 * p_.copy_to_length + 1 > length_) p_.copy_to_length = (length_ - 1) / 2;
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

void World::kill_random_shadow() {
  const int del = rng_.random(shadow_count());
  const int last = shadow_count() - 1;
  shadow_genotypes_[del] = std::move(shadow_genotypes_[last]);
  shadow_ages_[del] = shadow_ages_[last];
  shadow_birth_times_[del] = shadow_birth_times_[last];
  shadow_genotypes_.pop_back();
  shadow_ages_.pop_back();
  shadow_birth_times_.pop_back();
}

std::unique_ptr<Organism> World::remove_real(int cell) {
  std::unique_ptr<Organism> o = std::move(grid_[cell]);
  kill_random_shadow();
  return o;
}

void World::add_shadow(Genotype g) {
  shadow_genotypes_.push_back(std::move(g));
  shadow_ages_.push_back(0);
  shadow_birth_times_.push_back(ga_time_);
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

void World::reset_shadow_to_real() {
  int k = -1;
  for (int at = 0; at < size_; ++at) {
    const Organism* o = grid_[at].get();
    if (!o) continue;
    ++k;
    shadow_genotypes_[k] = o->genotype();
    shadow_ages_[k] = o->age;
    shadow_birth_times_[k] = o->birth_time;
  }
}

void World::terminate() {
  for (int d = 0; d < size_; ++d)
    if (grid_[d]) remove_real(d).reset();
}

}  // namespace oee::geb
