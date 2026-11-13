// Port of Geb v08b3 geb_core: toroidal world, agents, biotic interactions, and the
// random-selection shadow population. See docs/GEB_SPEC.md sections 5-6.
#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "geb/ann.hpp"
#include "geb/rng.hpp"

namespace oee::geb {

struct Params {
  int world_length = 20;
  int max_num_nodes = 20;
  int min_num_individuals = 20;   // -1: none
  int max_num_individuals = -1;   // -1: world size
  int start_num_individuals = 50; // -1: world size
  int copy_to_length = 3;         // input neighbourhood radius
  int seed = 0;                   // 0: from clock
  // GebR negative control: genetic parents and victims chosen at random among real
  // agents (event timing and placement still from agent behaviour). Not in Geb.
  bool random_selection = false;
  // Second, independent reset shadow on its own RNG stream (null control for the
  // normalised activity test). Does not change the real run or the first shadow.
  bool second_shadow = false;
  // M5 coupling (docs/M5.md). 0: Geb's built-in crossover + mutation. 1: heredity by
  // executing a parent's genome as BFF code; the child receives what the code writes
  // into a random buffer. 2: same with a zero buffer (v2). 3: v1 plus viability (a child
  // is born only if its own code copies >= 16 informative bytes of itself). Not in Geb. Fits in padding: sizeof(Params) stays 32.
  std::uint8_t heredity = 0;
  // L3 collectives (docs/L3.md). 0: Geb. 1: kin colonies. Bond/unbond actions (output
  // prefixes "000", "001", unused in Geb) act only within a colony and within 2 cells;
  // budding: a child joins its parent's colony and bonds to it when the parent's bond
  // output was active and the parent's group has fewer than 32 members (else it founds a
  // new colony); no killing within a group; an attack succeeds only if the attacker's
  // group is at least as large as the victim's. Not in Geb.
  // Bit flags: 1 = kin-colony rules; 2 = control (grouping decisions are coin flips);
  // 4 = senescence (death after kMaxAge updates). Values used: 1, 2, 5, 6, and 4 alone
  // (plain Geb plus senescence: no colony rules, a control).
  // 8 = L3 v5 colony reproduction (with 1 or 2): a child that is not budded founds a new
  // colony (single-agent propagule) only if its parent's group has >= kPropaguleMin
  // members; otherwise it stays in the parent's colony, unbonded. No killing within a
  // colony (bonded or not). A colony with >= kColonyMax members founds a new colony with
  // every birth. New colonies are thus offspring of grown groups; colony size is capped.
  // 16 = L3 v6 division of labour (with 8): an agent's role is warrior if it fires kill
  // more often than reproduce (moving averages), else breeder. A group's combat power is
  // its size, doubled if it has >= kDolMin members and 25-75% warriors. An attack succeeds
  // only if the attacker's power is >= the victim's. Mixed groups win fights.
  std::uint8_t collectives = 0;
};

static_assert(sizeof(Params) == 32, "Params is stored raw in checkpoints; keep its size");

class World;

class Organism : public Ann {
 public:
  Organism(World& w, Genotype genotype, int at_x_64, int at_y_64);
  // Restore from a checkpoint (consumes no randomness).
  struct LoadTag {};
  Organism(World& w, Genotype genotype, LoadTag);
  ~Organism();

  void copy_net_inputs_from_neighbourhood();
  void actions();

  std::uint64_t id;
  int pen_colour = -1;
  int at = 0, at_x = 0, at_y = 0;
  int at_x_64 = 0, at_y_64 = 0;
  int direction_deg = 0;
  long birth_time = 0;
  int age = 0;
  // L3: ids of bonded agents (at most kMaxBonds; dead ids are pruned lazily).
  std::vector<std::uint64_t> bonds;
  bool bond_out = false;
  std::uint64_t colony = 0;  // L3: kin colony id (founder's id); bonds only within a colony
  // L3 v6: exponential moving averages of reproduce / kill firing (fixed point, 1<<16 = 1).
  int ema_rep = 0, ema_kill = 0;
  bool warrior() const { return ema_kill > ema_rep; }

 private:
  friend class World;
  int action_output(const GeneSpan& prefix);
  void try_to_reproduce_with_organism_in_front();
  void try_to_kill_organism_in_front();
  void turn_anti_clockwise(int by_deg);
  void turn_clockwise(int by_deg);
  void try_to_move_forward(int distance_64);
  void try_to_bond_with_organism_in_front();
  void unbond_all();

  World& w_;
};

class World {
 public:
  explicit World(const Params& p);
  ~World();

  // Full state (parameters, RNG streams, agents, shadows, scheduling).
  void save(Writer& w) const;
  static std::unique_ptr<World> load(Reader& r);
  // Restart the random streams (both shadows' streams) from seed s: the run continues
  // on a different stochastic branch from the current state.
  void reseed(int s);
  World(const World&) = delete;
  World& operator=(const World&) = delete;

  // One call processes one agent slot (ga_main_loop).
  void step();
  // Delete all agents (ga_terminate). Consumes RNG as the original does.
  void terminate();

  const Params& params() const { return p_; }
  int seed_used() const { return seed_used_; }
  long time() const { return ga_time_; }
  long num_reproductions() const { return num_reproductions_; }
  int world_length() const { return length_; }
  // L3 v7: enlarge the torus to length L (> current). Agents keep their coordinates; new
  // cells are empty. Call only at a cycle boundary (right after time() changes).
  void grow(int L);
  // L3 v8: raise the neuron limit for agents developed from now on (existing agents keep theirs).
  void set_max_num_nodes(int n) { if (n > p_.max_num_nodes) p_.max_num_nodes = n; }
  int world_size() const { return size_; }
  const Organism* at(int cell) const { return grid_[cell].get(); }
  int num_individuals() const { return num_individuals_; }
  // L3 group statistics: number of groups, largest group, mean group size, fraction of
  // agents with at least one live bond.
  struct GroupStats { int groups = 0, largest = 0, colonies = 0, largest_colony = 0; int large = 0, mixed = 0; double mean = 0, bonded = 0, warrior_fraction = 0; };
  GroupStats group_stats() const;
  static constexpr int kMaxBonds = 4;
  static constexpr int kBondRange = 2;  // cells (toroidal Chebyshev); farther bonds break
  static constexpr int kColonyMax = 32;
  static constexpr int kPropaguleMin = 4;
  static constexpr int kDolMin = 4;  // L3 v6: minimum group size for the mixed-role bonus
  static int combat_power(const std::vector<const Organism*>& g);  // L3 v5: minimum group size to found a colony
  long propagules() const { return propagules_; }
  static constexpr int kMaxAge = 1000;  // L3 v4 senescence (collectives bit 2)  // a budded child joins the parent's group only below this size

  // Shadow population (genotypes only).
  int shadow_count() const { return static_cast<int>(shadow_genotypes_.size()); }
  const Genotype& shadow_genotype(int k) const { return shadow_genotypes_[k]; }
  // Reset shadow k-th to a copy of the k-th real agent in cell order (resets variant).
  void reset_shadow_to_real();
  // Replace every real and shadow genotype by founders from `pool` (cycled). Used once at
  // start for the M5 coupling; consumes randomness through Organism construction.
  void seed_genomes(const std::vector<Genotype>& pool);
  const std::vector<Genotype>& second_shadow() const { return shadow2_; }

  Ran1& rng() { return rng_; }
  const NetConstants& net() const { return net_; }
  int cosdeg64(int t) const { return cos64_[t]; }
  int sindeg64(int t) const { return sin64_[t]; }

 private:
  friend class Organism;
  struct Empty {};
  World(const Params& p, Empty);
  void init_tables();
  void init_wrap();
  int forward1(int from_x_64, int from_y_64, int direction_deg) const;
  // Delete the agent at `cell` and a uniformly random shadow.
  std::unique_ptr<Organism> remove_real(int cell);
  void kill_random_shadow();
  int random_occupied_cell(int exclude);
  // Breed real and shadow children; returns the real child (not yet placed) and
  // the shadow child genotype (not yet appended).
  std::unique_ptr<Organism> reproduce(Organism* a, Organism* b, int child_x_64, int child_y_64,
                                      Genotype& shadow_child);
  void add_shadow(Genotype g);
  Genotype breed_second_shadow();
  // M5: child genotype = buffer after executing [g1 | random buffer] as BFF code.
  static Genotype bff_breed(Ran1& r, const Genotype& g1, const Genotype& g2, int heredity);
  // M5 v3: does g's own code pass on at least 16 informative bytes of itself?
  static bool bff_viable(Ran1& r, const Genotype& g);
  static Genotype bff_breed_viable(Ran1& r, const std::vector<const Genotype*>& pool);
  void reset_not_processed();
  // L3 helpers.
  Organism* find_by_id(std::uint64_t id) const;
  void prune_bonds(Organism* o) const;
  int group_size(const Organism* o, std::vector<const Organism*>* members = nullptr) const;
  static void bond(Organism* a, Organism* b);
  bool in_bond_range(const Organism* a, const Organism* b) const;
  std::unordered_map<std::uint64_t, Organism*> byid_;  // live agents (L3 lookups)

  Params p_;
  Ran1 rng_;
  int seed_used_ = 0;
  NetConstants net_;
  int length_, size_;
  int cos64_[360], sin64_[360];
  std::vector<std::unique_ptr<Organism>> grid_;
  std::vector<Genotype> shadow_genotypes_;
  std::vector<int> shadow_ages_;
  std::vector<long> shadow_birth_times_;
  Ran1 rng2_;
  std::vector<Genotype> shadow2_;
  Genotype pending_shadow2_;
  std::vector<int> occupied_;
  std::vector<int> wrap_;
  std::vector<int> np_positions_;
  std::vector<std::uint64_t> np_ids_;
  int num_not_processed_ = 0;
  long propagules_ = 0;  // L3 v5: colonies founded (cumulative, checkpointed when bit 8 set)
  long ga_time_ = -1;
  long num_reproductions_ = 0;
  int num_individuals_ = 0;
  std::uint64_t next_id_ = 1;

  static constexpr int kTA = 500;
  static constexpr int kMaxNoiseOnIsolatedOutputs = 500;
  static constexpr int kMaxTurnDeg = 60;
  static constexpr int kCopyScale64AtDistance1 = 256 << 6;
  static constexpr int kGenotypeLengthChangeProb = 100;
  static constexpr int kGenotypeLengthChangeMax = 1;
  static constexpr int kNumMutations = 1;
};

}  // namespace oee::geb
