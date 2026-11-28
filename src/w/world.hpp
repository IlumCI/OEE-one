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
                   // own 2: the same, but copies are wild (no owner): only what an agent makes is private.
  // W3.3 split-role nodes (signals): solid food sources, ripe node_ripe ticks per node_per (own phase each), whose
  // state is visible only from within `view`. A ripe node pays node_pay to each agent eating from it, but only when
  // at least two agents eat from it in the same tick; then it is spent until its next cycle.
  int nodes=0,node_pay=150,node_per=200,node_ripe=40;  // nodes: count per 1024 cells
  // Wave 2 (old or disputed ideas, docs/WORLD.md): kin_sig 1 = symbols are heard only by the sender's own lineage
  // (cheap talk with common interest, Crawford & Sobel 1982); lamarck 1 = a newborn starts with its parent's recurrent
  // state instead of a blank one (inheritance of acquired memory; applied by the caller using `parents`)
  int kin_sig=0,lamarck=0;
  int wall_decay=0;  // per tick, a wall becomes empty with probability wall_decay/65536 (erosion; 0 = walls are permanent)
  // Machine economy (calibration, docs/WORLD.md): a running machine pays 1 store every m_cost ticks (1: every tick);
  // C moves m_give store from the parent into the copy (0: copies start empty and halt)
  int m_cost=1,m_give=0;
  // graze 1: eat also takes from the store of the machine in front, after the agent's own cell: any machine without
  // ownership; with own, wild machines (no owner) and the agent's own (0: only the own-granary rule)
  int graze=0;
  // pass 1: agents can stand on machine cells (walls and nodes still block), and eating then also grazes the machine
  // underfoot (same rights as graze). Lets consumers reach the inside of a colony (keystone predation, Paine 1966).
  int pass=0;
  // W4a irrigation (instruction set): irr>0 turns ',' into I: raise the irrigation level of the machine's own cell by
  // irr (cap 255) for 1 store. A cell's capacity is min(255, natural/seasonal capacity + irrigation); irrigation
  // decays by 1 every irr_decay ticks.
  int irr=0,irr_decay=16;  // irr_decay: ticks per unit of irrigation decay
  int claim=0;  // W4b: 1 = feeding a wild machine claims it for the feeder's lineage (homesteading)
  int land=0;   // W4d: 1 = a cell a machine irrigates becomes land of the lineage that last fed that machine; while it
                // is irrigated only that lineage eats its food (needs irr); ownership lapses when irrigation decays to 0
  // W4e homesteads (diagnostic; needs land): every farm_every ticks each lineage with living agents gets `farms` farm
  // machines (16 irrigation instructions, owned and last fed by it) on fixed cells of its own land, at full irrigation,
  // with farm_store store (a standing farm keeps its tape and is topped up)
  int farms=0,farm_store=40,farm_every=600;
  int farm_free=0;  // W4e: 1 = homestead farms pay no running cost (store is spent only on irrigation)
  int irr_full=0;   // 1 = an irrigation step spends store only when the whole +irr fits under the cap of 255
  int home_cue=0;   // W4g: observation entries 123-126 give the egocentric offset (forward, right; 8 per cell) to the
                    // agent's nearest own homestead farm cell, that farm's store/2 and its land's irrigation/2
  int farm_reset=1; // W4e 1: every placement tops farms up and re-irrigates their land; W4f 0: only the first placement
                    // (t 0) does; later ones rebuild destroyed farms empty, and homestead farms never decay
  // IN1 (docs/SOCIAL.md): give 1 = action "give" (index 7 without matter, needing nact 8; index 10 with matter, nact 11):
  // up to give_amt energy to the kin agent in front, keeping 1. Observation entry 63 (din 64) or 127 (din 128) shows the
  // energy/8 of the agent in front if it is kin (a cue of need).
  int give=0,give_amt=20;
  // AR1 (docs/ARTIFACTS.md; needs signals, no matter): art = artifacts per 1024 cells, puzzle boxes agents cannot enter.
  // Each shows a glyph (0..art_glyphs-1) and hides key = law[glyph] (1-3 tokens from symbols 1..3, P(k) ~ 2^-k). An
  // artifact reads the symbol of the first agent (tick order) facing it: a matching token advances its lock, a wrong
  // non-zero one resets it, silence leaves it; a full key pays art_pay energy to that agent and the artifact rests
  // art_rest ticks. The law is redrawn every art_era ticks (0: never; locks reset); art_family 1 draws each era's law
  // from a hidden family (a fixed base table under a token shift and reversal: a law of the laws, AR3).
  // art_lock: a wrong token also locks the artifact for art_lock ticks (guessing is costly; 0: no lockout)
  int art=0,art_glyphs=8,art_pay=60,art_rest=100,art_era=1000,art_family=0,art_lock=0;
  // art_code 1: the ready artifact in front is shown as one line of 3*art_glyphs (glyph x lock progress) at 104.. (64),
  // a categorical sense of the lock state, instead of the scalars at 104 (progress) and 105 (rest); all zero if resting
  int art_code=0;
  // LL1c (docs/LIFE.md; worlds without signals or matter): kinds K in 1..4: the food of every cell is of one of K kinds
  // (fixed per cell, from the seed); in each era of kind_era ticks every kind is nutritious or toxic (drawn from the
  // seed and the era; at least one nutritious); eating toxic food costs kind_tox energy per unit eaten instead of
  // giving one. Seen as one-hot lines (64) for the food underfoot (53..52+K) and in front (53+K..52+2K)
  // kind_bad b >= 1: instead, exactly b kinds are toxic in each era (drawn from the seed and the era), so a naive
  // eater keeps a viable share of its intake (from random founders)
  int kinds=0,kind_era=1000,kind_tox=1,kind_bad=0;
  // AR5 (docs/ARTIFACTS.md): art_kmax caps the key length of artifacts (1..3; 3 = AR1 unchanged)
  int art_kmax=3;
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
  std::uint32_t deaf_kin=0,deaf_other=0;  // analysis: lineages that hear no symbols from their own / from other lineages
  std::uint32_t nofeed=0;              // analysis: lineages whose feed action does nothing (not saved)
  std::uint32_t nocue=0;               // analysis: lineages that see zeros in the W4g cue entries (not saved)
  // analysis: per-lineage counters when track is set (not saved): stat[k*32+a] actions a = 0..15, +16 kills made,
  // +17 births, +18 starved, +19 died of age, +20 killed, +21 non-zero symbols, +22 agent-ticks, +23 joint harvests,
  // +29 agent-ticks on own irrigated land (with land); +30 energy given to kin (with give);
  // with irr (credit for irrigation, k = the credited lineage): +24 food from k-credited irrigated cells eaten by k,
  // +25 the same eaten by other lineages, +26 food from uncredited irrigated cells eaten by k, +27 energy k fed into
  // machines, +28 all food k took from cells
  bool track=false;std::vector<long> stat;
  // analysis (probe; not saved), per slot for the last tick: pact the action drawn (-1: not alive at the start), peff
  // its effect (0 none; fwd 1 moved; turns 1; eat food taken; repro 1 born; attack energy taken; write 1; make/unmake
  // 1 made, 2 unmade, 3 destroyed; feed energy fed), pdie how the agent died in the tick (0 no, 1 starved, 2 age, 3 killed)
  bool probe=false;std::vector<std::int16_t> pact,peff;std::vector<std::uint8_t> pdie;
  // analysis (not saved): lineages whose futile draws are drawn again from the other actions with the same policy
  // (gate_eat: an eat that would take nothing; gate_all: any action that would have no effect, see futile())
  std::uint32_t gate_eat=0,gate_all=0;
  std::uint32_t sharp=0,greedy=0;  // analysis (not saved): lineages that sample with P(a) ~ 2^(logit/8) / take the argmax
  std::uint32_t nogive=0;          // analysis (not saved): lineages whose give action does nothing
  std::uint32_t artmute=0;         // analysis (not saved): lineages whose symbols artifacts ignore
  std::uint32_t artoracle=0;       // analysis (not saved): lineages that, facing a ready artifact, speak its next key token
  // analysis (not saved), the value of information ladder: lineages that, facing a ready artifact, always speak:
  // artguess a random token; artmemo the token their own memory knows for the lock state (glyph, progress), else an
  // untried one (a perfect individual learner; forgets a known token when it fails); artmemol the same with one memory
  // shared by the whole lineage (perfect pooled, cultural, memory)
  std::uint32_t artguess=0,artmemo=0,artmemol=0;std::vector<std::int8_t> amem;std::vector<std::uint64_t> amid;
  // LL1c per cell (with p.kinds; saved): food kind. kstat (analysis, track): per lineage eats of nutritious, eats of
  // toxic food, energy lost to toxic food. Value of information (analysis, not saved): kindoracle lineages never eat
  // toxic food; kindmemo agents refuse a kind their own last eat of it found toxic (retesting 1 time in 16); kindmemol
  // the same with one memory per lineage. A refused eat becomes a forward move (moving on, not standing on the food)
  // kage (analysis, track): per lineage 6 bins x (nutritious eats, toxic eats); bins 0-4 eaters born in the current
  // kind era by age/100, bin 5 eaters born in an earlier era (the learning curve, and relearning after a change)
  std::vector<std::uint8_t> fkind;std::vector<long> kstat,kage;std::uint32_t kindoracle=0,kindmemo=0,kindmemol=0;std::vector<std::int8_t> kmem;std::vector<std::uint64_t> kmid;
  bool ktox(int kind,long era) const;  // LL1c: kind is toxic in era
  std::int8_t* kmrow(int s);           // LL1c analysis: the memory row (per kind: 0 unknown, 1 nutritious, 2 toxic) of slot s
  // analysis (track, not saved): per lineage 6 bins x (tokens, openings, resets): bins 0-4 speakers born in the current
  // era by age/100, bin 5 speakers born in an earlier era (within-life learning of the law, and relearning after a change)
  std::vector<long> aage;
  // AR1 per cell (with p.art; saved): artifact glyph (-1 none), lock progress, rest ticks left; astat (analysis, track):
  // per lineage tokens spoken to artifacts, openings, resets, energy gained
  std::vector<std::int8_t> aglyph;std::vector<std::uint8_t> aprog;std::vector<std::int16_t> arest;std::vector<long> astat;
  bool is_art(int c) const{return !aglyph.empty()&&aglyph[c]>=0;}
  void art_key(int g,long era,int* key,int& k) const;  // AR1: the key of glyph g in era (deterministic from the seed)
  int give_act() const{return p.matter?10:7;}  // index of the give action (with p.give)
  bool futile(int s,int a) const;  // analysis: action a of the agent in slot s would have no effect in the current state
  std::vector<std::int8_t> mfd,ilin;  // last feeder lineage per machine (world state with land, else analysis); lineage
                                      // credited for each cell's irrigation (analysis, track)
  std::vector<std::int8_t> lown;      // W4d: owner lineage of each cell's land, -1 common (with land; saved)
  std::vector<std::uint8_t> hfm;      // W4e: 1 = the machine on this cell is a homestead farm (with farms; saved)
  // W3 matter, per cell (allocated when p.matter): material 0 empty, 1 machine, 2 wall; machine tape (16 bytes),
  // instruction pointer, heads, direction, store, halted ticks, creator lineage, birth tick, effect counters
  // (4 per cell: resource harvested, resource delivered, copies made, walls changed)
  std::vector<std::uint8_t> mat,tape,mip,mh0,mh1,mdir;std::vector<std::int16_t> mst;std::vector<std::uint16_t> mhalt;
  std::vector<std::int8_t> mlin;std::vector<long> mbt;std::vector<std::int32_t> mfx;
  std::vector<std::int32_t> nph,ncyc;  // W3.3 nodes (material 3), per cell: phase, last cycle harvested
  std::vector<std::uint8_t> irl;       // W4a: irrigation level per cell (with p.irr)
  std::vector<int> parents;            // after tick(): parent slot of each entry of `born` (same order; not saved)
  // extinction event (punctuated equilibrium; Lehman & Miikkulainen 2015): every agent outside a random square window
  // covering `keep` of the area dies (structures stay). Returns how many died.
  int catastrophe(std::uint64_t salt,double keep);
  int home_cell(int k,int i) const;  // W4e: cell of lineage k's i-th homestead farm
  void homestead();                  // W4e: place or top up every living lineage's homestead farms
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
  // with land: resource entries show 0 on another lineage's irrigated land; 122 the land underfoot (+64 own, -64 another
  // lineage's, 0 common). With home_cue: 123-126 as described at WParams::home_cue.
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
const std::int64_t* act_weights();  // the sampling weights: P(a) ~ act_weights()[logit+128] = round(16*2^((logit+128)/16))

}  // namespace oee::w
