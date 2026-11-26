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
  // W2a (docs/WORLD.md; all off by default, W0/W1 unchanged)
  int season=0;    // mean season period in ticks (0: static fertility); period drawn in [season/2, 3*season/2)
  int view=2;      // resources visible within this radius of the 5x5 window (2: all)
  int signals=0;   // 1: each agent also emits a symbol 0..3 (0 = silent) from logits 8..11; heard in the window
  int rot=1;       // with seasons: resource above the current capacity decays by rot per tick
  int sig_cost=0;  // energy per tick while emitting a non-zero symbol
  // W3 programmable matter (docs/WORLD.md §4; off by default, W0-W2a unchanged)
  int matter=0;    // 1: cells can be machines (16-byte tapes that run) or walls; agents write, make, unmake, feed
  int m_steps=8;   // instructions per machine per tick (paid with 1 store)
  int m_decay=200; // halted ticks before a machine decays to empty
  int mk_cost=10,wr_cost=1,feed_amt=20,rep_cost=16,wall_cost=32;
  int spawn=0;     // W3.1: new random programs per 1000 ticks per 1024 cells (spontaneous generation)
  int mut=0;       // W3.1: copy error per copied byte, in 1/65536 (a wrong byte becomes a random instruction)
  int own=0;       // W3.2: machines belong to their maker's lineage (copies too). Only the owner can unmake one and
                   // recover its store; others can only destroy it (cost 2*mk_cost, store lost). The owner also eats
                   // from the store of its own machine in front once its cell is empty (a private granary).
  // W3.3 split-role nodes (signals): solid food sources, ripe node_ripe ticks per node_per (own phase each), whose
  // state is visible only from within `view`. A ripe node pays node_pay to each agent eating from it, but only when
  // at least two agents eat from it in the same tick; then it is spent until its next cycle.
  int nodes=0,node_pay=150,node_per=200,node_ripe=40;  // nodes: count per 1024 cells
  // Wave 2 (old or disputed ideas, docs/WORLD.md): kin_sig 1 = symbols are heard only by the sender's own lineage
  // (cheap talk with common interest, Crawford & Sobel 1982); lamarck 1 = a newborn starts with its parent's recurrent
  // state instead of a blank one (inheritance of acquired memory; applied by the caller using `parents`)
  int kin_sig=0,lamarck=0;
  int wall_decay=0;  // per tick, a wall becomes empty with probability wall_decay/65536 (erosion; 0 = walls are permanent)
};

struct Agent{int x=0,y=0,dir=0,energy=0,age=0,lineage=0;bool alive=false;std::uint64_t id=0;int sig=0,wpos=0;};

// W3 machine instruction bytes (BFF core plus world operations); every other byte does nothing
extern const std::uint8_t kCode[16];

struct World{
  static constexpr int kDin=64;      // observation length (int8) without signals; din() is the length in use
  static constexpr int kV=16;        // logits per agent without matter; nv() is the number in use
  // actions: 0 noop, 1 forward, 2 left, 3 right, 4 eat, 5 reproduce, 6 attack (W1); W3: 7 write, 8 make/unmake,
  // 9 feed. p.nact in use. Logits: actions from 0, signals from sig_off(), W3 write codes 16..31.
  static constexpr int kActions=7;
  WParams p;long t=0;std::uint64_t next_id=1;
  std::vector<std::uint8_t> res,fert;  // per cell
  std::vector<std::uint16_t> racc;     // per cell regrowth accumulator (fractional regrowth)
  std::vector<int> occ;                // per cell: slot index or -1
  std::vector<Agent> ag;               // K*S slots
  std::vector<std::uint8_t> fp;        // seasons: fertility of each patch separately (patches x cells)
  int per=0;std::vector<int> ph;       // seasons: current period and per-patch phase (ticks)
  std::uint32_t deaf=0;                // analysis: lineages (bit k) that hear no symbols (not saved)
  // analysis: per-lineage counters when track is set (not saved): stat[k*32+a] actions a = 0..15, +16 kills made,
  // +17 births, +18 starved, +19 died of age, +20 killed, +21 non-zero symbols, +22 agent-ticks
  bool track=false;std::vector<long> stat;
  // W3 matter, per cell (allocated when p.matter): material 0 empty, 1 machine, 2 wall; machine tape (16 bytes),
  // instruction pointer, heads, direction, store, halted ticks, creator lineage, birth tick, effect counters
  // (4 per cell: resource harvested, resource delivered, copies made, walls changed)
  std::vector<std::uint8_t> mat,tape,mip,mh0,mh1,mdir;std::vector<std::int16_t> mst;std::vector<std::uint16_t> mhalt;
  std::vector<std::int8_t> mlin;std::vector<long> mbt;std::vector<std::int32_t> mfx;
  std::vector<std::int32_t> nph,ncyc;  // W3.3 nodes (material 3), per cell: phase, last cycle harvested
  std::vector<int> parents;            // after tick(): parent slot of each entry of `born` (same order; not saved)
  // extinction event (punctuated equilibrium; Lehman & Miikkulainen 2015): every agent outside a random square window
  // covering `keep` of the area dies (structures stay). Returns how many died.
  int catastrophe(std::uint64_t salt,double keep);
  bool ripe(int c) const;
  int din() const{return (p.signals||p.matter)?128:64;}
  int nv() const{return p.matter?32:16;}
  int sig_off() const{return p.matter?12:8;}
  bool ext() const{return p.season>0||p.signals>0||p.view!=2||p.matter>0;}  // W2a/W3 state present
  int wipe(int k);  // analysis: machines made by lineage k (and their copies) become empty; returns how many
  // test arenas (W3 generality): place n machines with random programs on empty cells; program length 1-16 with
  // P(length > l) = cont^l (shorter programs are likelier, as in a simplicity prior), instructions uniform over kCode;
  // store `store`, no owner lineage. Returns how many were placed.
  int seed_programs(int n,std::uint64_t salt,int store,double cont);
  // intervention on the world's mechanisms (swap test): remove every machine without an owner lineage (seeded
  // programs and their copies) and seed the same number of new random programs. Returns how many were swapped.
  int swap_programs(std::uint64_t salt,int store,double cont);
  // census of machines that lived at least min_age ticks: machines, walls, distinct tapes, distinct effect classes
  void census(int min_age,int& machines,int& walls,int& tapes,int& classes) const;
  void set_climate(std::uint64_t salt);  // seasons: draw a new period and phases
  int cap(int c) const;                  // current resource capacity of cell c (fert[c] when static)
  void init(const WParams& p,int founders_per_lineage);
  void reseed_lineage(int k,int founders,std::uint64_t salt);  // place fresh founders of lineage k (extinction recovery)
  // din() int8 values (zeros for empty slots). 0-24: resource in the egocentric 5x5 window (0 beyond `view`);
  // 25-49: other agents (+64 same lineage, -64 other); 50 energy/8; 51 age/4; 52 bias 64;
  // with signals: 53-77 symbol*32 of the agent in each window cell (0 when deaf); 78 own symbol*32.
  // with matter: 79-103 material of each window cell (machine +48, wall -48, node +96; a node within view shows 127 in
  // the resource entries 0-24 while ripe, else 0); 104-119 tape of the machine in front
  // (instruction index*8+4, data -64); 120 its store/4; 121 own write position*8.
  void observe(int slot,std::int8_t* o) const;
  // One tick. logits[k] points to S x kV int8 logits of lineage k (row s = slot k*S+s). Slots of newborn agents are
  // appended to `born` (their recurrent state must be reset by the caller).
  void tick(const std::int8_t* const* logits,std::vector<int>& born);
 private:
  void make_machine(int c,int dir,int lin);
  void kill_machine(int c);
  int front_of(int c,int dir) const;
  void run_machines();
 public:
  long biomass(int k) const;
  int count(int k) const;
  long resources() const;
  void move_slot(int from,int to);  // relabel an agent into another (empty) slot, e.g. on a lineage split
  void save(std::FILE* f) const;     // full state, field by field (equal states give equal bytes); W1 format unless ext()
  bool load(std::FILE* f);
};

int act_of(const std::int8_t* logit,int n);  // argmax over the first n logits, ties to the lowest index
// sample over the first n logits with P(a) ~ 2^(logit/16), from 64 random bits
int act_sample(const std::int8_t* logit,std::uint64_t bits,int n);

}  // namespace oee::w
