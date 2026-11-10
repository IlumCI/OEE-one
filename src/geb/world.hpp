// Port of Geb v08b3 geb_core: toroidal world, agents, biotic interactions, and the
// random-selection shadow population. See docs/GEB_SPEC.md sections 5-6.
#pragma once

#include <cstdint>
#include <memory>
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
  // into a buffer. Not in Geb. Fits in padding: sizeof(Params) stays 32.
  std::uint8_t heredity = 0;
};

static_assert(sizeof(Params) == 32, "Params is stored raw in checkpoints; keep its size");

class World;

class Organism : public Ann {
 public:
  Organism(World& w, Genotype genotype, int at_x_64, int at_y_64);
  // Restore from a checkpoint (consumes no randomness).
  struct LoadTag {};
  Organism(World& w, Genotype genotype, LoadTag);

  void copy_net_inputs_from_neighbourhood();
  void actions();

  std::uint64_t id;
  int pen_colour = -1;
  int at = 0, at_x = 0, at_y = 0;
  int at_x_64 = 0, at_y_64 = 0;
  int direction_deg = 0;
  long birth_time = 0;
  int age = 0;

 private:
  int action_output(const GeneSpan& prefix);
  void try_to_reproduce_with_organism_in_front();
  void try_to_kill_organism_in_front();
  void turn_anti_clockwise(int by_deg);
  void turn_clockwise(int by_deg);
  void try_to_move_forward(int distance_64);

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
  int world_size() const { return size_; }
  const Organism* at(int cell) const { return grid_[cell].get(); }
  int num_individuals() const { return num_individuals_; }

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
  static Genotype bff_breed(Ran1& r, const Genotype& g1, const Genotype& g2);
  void reset_not_processed();

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
