// EGG: integer recurrent language model trained by integer EGGROLL (docs/EVO.md).
// Implemented from the specification in Sarkar et al., "Evolution Strategies at the Hyperscale"
// (arXiv:2511.16652v2), Appendix G (architecture) and Appendix H (integer EGGROLL): all weights int8,
// int32 accumulation, no activation functions (saturating int8 casts are the nonlinearity), rank-1
// antithetic perturbations, sign fitness shaping, thresholded one-bin parameter updates.
#pragma once
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace oee::evo {

// layers, width (power of 4), outputs (byte vocabulary, or actions; multiple of 16), sigma = 2^-sig,
// Din: 0 = byte-embedding input (language model), > 0 = int8 observation vector of that size (multiple of 64)
struct EggCfg{int L=6,D=256,V=256,sig=4,Din=0;};

struct Mat{int m=0,n=0,id=0;std::vector<std::int8_t> w,p;};  // w: m x n (out, in); p: packed w^T for gemm
struct Vec{int n=0,id=0;std::vector<std::int8_t> w;};

struct Egg{
  EggCfg c;
  Mat emb,inp,head;Vec lnout;  // emb (V x D) when Din == 0, inp (D x Din) when Din > 0
  struct Layer{Vec ln1,ln2,bf,bh;Mat Wf,Uf,Wh,Uh,m1,m2;};
  std::vector<Layer> ly;
  std::vector<Mat*> mats;std::vector<Vec*> vecs;  // registries (ids index the noise streams)
  explicit Egg(const EggCfg& c,std::uint64_t seed);
  Egg(const Egg&)=delete;Egg& operator=(const Egg&)=delete;
  long nparams() const;
  void repack();  // after any weight change
  void save(const std::string& fn,long step) const;
  long load(const std::string& fn);  // returns the saved step
  void save(std::FILE* f,long step) const;  // same record, appended to an open file (checkpoints)
  long load(std::FILE* f);
};

// Rank-1 noise for one training step: per pair p and parameter k, int8 vectors A (length m or n) and B (length n).
struct Noise{
  int P=0;std::vector<std::size_t> offA,offB;std::vector<int> lenA,lenB;std::vector<std::int8_t> buf;
  void gen(const Egg& g,int P,std::uint64_t seed,long step);
  const std::int8_t* A(int k,int p) const{return buf.data()+offA[k]+(std::size_t)p*lenA[k];}
  const std::int8_t* B(int k,int p) const{return buf.data()+offB[k]+(std::size_t)p*lenB[k];}
};

// Rows: each row is a persistent stream with its own hidden state, a pair index and a sign (+1, -1, or 0 = no noise).
struct Rows{
  int R=0;std::vector<int> pair,sign;std::vector<std::int8_t> state;  // state: R x L x D
  void init(const Egg& g,int R);
};

// Run T tokens for all rows: tok[r][t] are inputs, tok[r][t+1] targets (row-major R x (T+1)).
// Adds per-row log2-likelihood (in 1/16 bits) to fit[r]. Multithreaded over row blocks of 32.
void run_rows(const Egg& g,const Noise* nz,Rows& rows,const std::uint8_t* tok,int T,std::int64_t* fit,int threads);

// Integer EGGROLL update from per-pair shaped fitness F[p] in {-1,0,1}; alpha = fraction of parameters to move.
// Returns the number of parameters changed.
long egg_update(Egg& g,const Noise& nz,const std::vector<int>& F,double alpha);

// Single-threaded stepping over rows [r0, r0+n) of `rows`: one forward step per call, recurrent state kept in rows.state.
// Construct after setting rows.sign / rows.pair (perturbed vector parameters are cached per row).
class Stepper{
 public:
  Stepper(const Egg& g,const Noise* nz,Rows& rows,int r0,int n);
  ~Stepper();
  Stepper(const Stepper&)=delete;Stepper& operator=(const Stepper&)=delete;
  // obs: n x Din (Din > 0); out: n x V int8 logits. Returns out.
  const std::int8_t* step(const std::int8_t* obs);
  // Step only the m rows listed in idx (local indices in [0, n), any order): obs is m x Din, out m x V (compacted).
  // Rows not listed keep their recurrent state. Per-row results equal those of the full step.
  const std::int8_t* step(const std::int8_t* obs,const int* idx,int m);
 private:
  struct Impl;Impl* p_;
};

double egg_macs_per_token(const EggCfg& c);  // multiply-accumulates of one forward step for one row
int egg_fdiv(int x,int m);  // floor(x/m) as used by the integer layer norm (exposed for tests)

}  // namespace oee::evo
