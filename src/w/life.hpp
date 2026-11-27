// Lifetime learning (docs/LIFE.md, LL1): each agent has its own plastic synapses from the top-layer state of its
// network (D int8) and a bias input (64) to its action logits. They are zero at birth and change during life by a
// three-factor rule: eligibility trace (gradient of log pi of the chosen action) times reward prediction error (the
// change in the agent's own energy over the tick against its running mean), optionally scaled by a neuromodulator
// that the network itself outputs. Integer arithmetic throughout (bit-exact, resumable).
#pragma once
#include <cstdint>
#include <cstdio>
#include <vector>

namespace oee::w {

struct LifeCfg{
  int on=0;      // 1: learning on (0: synapses stay zero, the inherited policy only)
  int repro=0;   // 1: reproduction is plastic too; a birth's reward counts the energy given to the newborn as kept
  int mod=0;     // 1: the update is scaled by m = (logit[mlog] + 128) / 128 in [0, 2) (network-computed neuromodulator)
  int mlog=15;   // logit used as the neuromodulator (an otherwise unused output: 15 without matter, 11 with)
  int eta=10;    // learning rate 2^-eta (see Life::learn for the units)
  int lam=230;   // eligibility trace decay lam/256 per tick
  int beta=4;    // running-mean (baseline) decay 2^-beta per tick
  int teach=0;   // SL1 V (docs/SOCIAL.md): a newborn's synapses start at half its parent's (traces zero, baseline copied)
  int obs=0;     // SL1 O: each tick an agent also learns, at half rate, from its nearest kin neighbour's action and reward
  // plastic actions (bit a): all of the first na except reproduction (unless repro), give (IN1; index 7 without matter,
  // 10 with) and, with matter, write, make, feed
  std::uint32_t mask(int na,bool matter) const;
};

struct Life{
  int R=0,D=0,na=0,J=0;  // rows (agents), features, actions, inputs J = D + 1 (the bias last)
  std::vector<std::int32_t> W,e,b;  // R x na x J synapses (Q16: logit units per input unit) and traces; R baselines (Q8)
  void init(int R,int D,int na);
  void reset(int r);                            // a newborn in row r: synapses, traces and baseline to zero
  void copy_row(int rd,const Life& src,int rs);  // row rd of this = row rs of src (same D, na)
  void inherit(int rd,int rs);                   // SL1 V: row rd (a newborn) = half the synapses of row rs (its parent)
  // adds the plastic contribution to the logits l (first na) of row r, given its features h (D int8)
  void apply(const LifeCfg& c,std::uint32_t msk,int r,const std::int8_t* h,std::int8_t* l) const;
  // after the tick: l the logits the action was sampled from (V entries; l[c.mlog] is the neuromodulator), a the action
  // taken, dE the reward (energy change over the tick)
  void learn(const LifeCfg& c,std::uint32_t msk,int r,const std::int8_t* h,const std::int8_t* l,int a,int dE);
  // SL1 O: row r (features h, logits l) learns from a neighbour's action a and reward prediction error d (Q8), at half
  // rate and without a trace; only when a is plastic
  void learn_obs(const LifeCfg& c,std::uint32_t msk,int r,const std::int8_t* h,const std::int8_t* l,int a,std::int64_t d);
  void save(std::FILE* f) const;
  bool load(std::FILE* f);
};

}  // namespace oee::w
