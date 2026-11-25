// World v1 (docs/WORLD.md), phase W0: torus with patchy regrowing resources and agents of K lineages.
// Deterministic (counter RNG keyed by seed and tick), copyable (forks are plain copies).
// Agent slots: K*S, slot index = lineage*S + s. Brains live outside the world (src/evo Stepper); the world only
// produces observations and consumes action logits.
#pragma once
#include <cstdint>
#include <cstdio>
#include <vector>

namespace oee::w {

struct WParams{
  int L=32,S=64,K=1,patches=6;        // side, slots per lineage, lineages, resource patches
  int e0=200,ecap=1000,base=1,move=1,turn=0,bite=32;
  int repro_min=120,repro_give=50,repro_cost=20,max_age=500,regrow=4;  // regrowth per tick = fert*regrow/4096 (fractional)
  int stoch=1;                        // 1: sample actions with P(a) ~ 2^(logit/16) (smooth fitness for ES); 0: argmax
  int nact=6;                         // actions in use: 6 (W0) or 7 (W1: + attack)
  int atk_cost=2,atk_take=20;         // attack: attacker pays atk_cost, takes up to atk_take energy from the agent in front
  std::uint64_t seed=1;               // fertility map, founders, tick order
};

struct Agent{int x=0,y=0,dir=0,energy=0,age=0,lineage=0;bool alive=false;std::uint64_t id=0;};

struct World{
  static constexpr int kDin=64;      // observation length (int8)
  static constexpr int kV=16;        // logits per agent (first kActions used)
  static constexpr int kActions=7;   // 0 noop, 1 forward, 2 left, 3 right, 4 eat, 5 reproduce, 6 attack (W1); p.nact in use
  WParams p;long t=0;std::uint64_t next_id=1;
  std::vector<std::uint8_t> res,fert;  // per cell
  std::vector<std::uint16_t> racc;     // per cell regrowth accumulator (fractional regrowth)
  std::vector<int> occ;                // per cell: slot index or -1
  std::vector<Agent> ag;               // K*S slots
  void init(const WParams& p,int founders_per_lineage);
  void reseed_lineage(int k,int founders,std::uint64_t salt);  // place fresh founders of lineage k (extinction recovery)
  void observe(int slot,std::int8_t* o) const;                 // kDin int8 values (zeros for empty slots)
  // One tick. logits[k] points to S x kV int8 logits of lineage k (row s = slot k*S+s). Slots of newborn agents are
  // appended to `born` (their recurrent state must be reset by the caller).
  void tick(const std::int8_t* const* logits,std::vector<int>& born);
  long biomass(int k) const;
  int count(int k) const;
  long resources() const;
  void move_slot(int from,int to);  // relabel an agent into another (empty) slot, e.g. on a lineage split
  void save(std::FILE* f) const;     // full state, field by field (equal states give equal bytes)
  bool load(std::FILE* f);
};

int act_of(const std::int8_t* logit,int n);  // argmax over the first n logits, ties to the lowest index
// sample over the first n logits with P(a) ~ 2^(logit/16), from 64 random bits
int act_sample(const std::int8_t* logit,std::uint64_t bits,int n);

}  // namespace oee::w
