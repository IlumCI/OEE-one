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
  int tkeep=1;   // teaching fidelity: the newborn keeps 1 - 2^-tkeep of its parent's synapses (1: half; 3: 7/8)
  int obs=0;     // SL1 O: each tick an agent also learns, at half rate, from its nearest kin neighbour's action and reward
  int rew=0;     // LL2: the reward adds logit[rlog]/8 energy units, an internal reward the network computes (evolved)
  int rlog=14;   // logit used as the evolved reward (an otherwise unused output)
  int sym=0;     // 1: the symbol head (logits so..so+3, signals worlds) is plastic too: its own softmax, the same reward
  int seta=-1;   // the symbol head's learning rate 2^-seta (-1: eta)
  int symh=1;    // 0: the symbol head reads only the bias and the raw inputs, not the features h (a sparse code: specific)
  int xeta=-1;   // learning rate 2^-xeta of the synapses from the raw inputs (-1: the head's rate). A one-hot line carries
                 // |x|^2 = 4096 against about 1.6e5 for the 64 features, so at one rate it learns about 40 times slower
  int in0=0,in1=0;  // the synapses also read raw observation entries in0..in1-1 (as the network sees them), after the bias
  // plastic actions (bit a): all of the first na except reproduction (unless repro), give (IN1; index 7 without matter,
  // 10 with) and, with matter, write, make, feed
  std::uint32_t mask(int na,bool matter) const;
};

struct Life{
  // rows (agents), features, actions, inputs J = D + 1 + nx (h, the bias, then nx raw inputs), plastic logits nl (na
  // actions, then 4 symbols at logits so..so+3 with LifeCfg::sym)
  int R=0,D=0,na=0,J=0,nl=0,nx=0,so=8;
  std::vector<std::int32_t> W,e,b;  // R x nl x J synapses (Q16: logit units per input unit) and traces; R baselines (Q8)
  void init(int R,int D,int na,int ns=0,int nx=0,int so=8);  // ns 4: plastic symbol head
  void reset(int r);                            // a newborn in row r: synapses, traces and baseline to zero
  void copy_row(int rd,const Life& src,int rs);  // row rd of this = row rs of src (same D, na)
  void inherit(int rd,int rs,int keep=1);        // SL1 V: row rd (a newborn) = 1 - 2^-keep of the synapses of row rs (its parent)
  // adds the plastic contribution to the logits l of row r, given its features h (D int8) and raw inputs x (nx int8)
  void apply(const LifeCfg& c,std::uint32_t msk,int r,const std::int8_t* h,const std::int8_t* x,std::int8_t* l) const;
  void apply(const LifeCfg& c,std::uint32_t msk,int r,const std::int8_t* h,std::int8_t* l) const{apply(c,msk,r,h,nullptr,l);}
  // after the tick: l the logits the action was sampled from (V entries; l[c.mlog] is the neuromodulator), a the action
  // taken, dE the reward (energy change over the tick)
  void learn(const LifeCfg& c,std::uint32_t msk,int r,const std::int8_t* h,const std::int8_t* l,int a,int dE){learn8(c,msk,r,h,nullptr,l,a,-1,(std::int64_t)dE*256);}
  void learn8(const LifeCfg& c,std::uint32_t msk,int r,const std::int8_t* h,const std::int8_t* l,int a,std::int64_t r8){learn8(c,msk,r,h,nullptr,l,a,-1,r8);}
  // reward in Q8; sg the symbol emitted (0..3; -1 none), used by the symbol head
  void learn8(const LifeCfg& c,std::uint32_t msk,int r,const std::int8_t* h,const std::int8_t* x,const std::int8_t* l,int a,int sg,std::int64_t r8);
  // SL1 O: row r (features h, logits l) learns from a neighbour's action a and reward prediction error d (Q8), at half
  // rate and without a trace; only when a is plastic
  void learn_obs(const LifeCfg& c,std::uint32_t msk,int r,const std::int8_t* h,const std::int8_t* x,const std::int8_t* l,int a,std::int64_t d);
  void learn_obs(const LifeCfg& c,std::uint32_t msk,int r,const std::int8_t* h,const std::int8_t* l,int a,std::int64_t d){learn_obs(c,msk,r,h,nullptr,l,a,d);}
  void save(std::FILE* f) const;
  bool load(std::FILE* f);
};

}  // namespace oee::w
