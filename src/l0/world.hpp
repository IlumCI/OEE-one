// L0 physics: mass-conserving continuous CA with rule parameters carried by matter.
// Transport: MaCE (Papadopoulos & Guichard, arXiv:2507.12306): each cell splits its mass over its 3x3
// neighbourhood with weights exp(beta*A_q) normalised over that neighbourhood (drift up the affinity
// gradient plus diffusion). Affinity: Lenia growth over a global kernel bank, weighted per cell by the
// parameter vector P that the matter carries (Flow-Lenia parameter localisation, arXiv:2212.07906,
// eq. 8; incoming parameters are mass-weighted averages, eq. 9). New here: P mutates as matter moves,
// so the physics itself varies heritably inside the world.
// Mass is 64-bit fixed point; shares are split by largest remainder, so total mass is conserved exactly.
#pragma once
#include <cstdint>
#include <vector>

namespace oee::l0 {

constexpr std::int64_t kOne=1<<20;  // fixed-point mass unit

struct Kernel{double R=13,a[3]={.5,0,0},b[3]={1,0,0},w[3]={.15,.15,.15},mu=.15,sigma=.015;};

struct Params{
  int n=256;              // grid side, power of two
  double beta=6;          // MaCE drift strength
  std::vector<Kernel> bank{Kernel{}};
  double mut_p=0;         // per cell per step probability of a parameter mutation (0 = off)
  double mut_s=0.05;      // mutation step (Gaussian sd), P clamped to [0,1]
  bool sample=false;      // incoming parameters: false = mass-weighted average (eq. 9), true = one source
                          // chosen with probability proportional to its incoming mass (eq. 10; no blending)
  bool neutral=false;     // control: P is carried and mutated but the affinity uses a fixed shared vector
                          // (the mass-weighted mean P at the first step), so P cannot affect the physics
  std::vector<double> fixedP;  // neutral: shared vector to use instead of the first-step mean (optional)
  std::uint64_t seed=1;
};

class World{
 public:
  explicit World(const Params& p);
  const Params& params()const{return p_;}
  int n()const{return p_.n;}
  int nk()const{return (int)p_.bank.size();}
  long time()const{return t_;}
  std::int64_t total()const;
  std::uint64_t hash()const;
  std::vector<std::int64_t>& mass(){return m_;}
  const std::vector<std::int64_t>& mass()const{return m_;}
  std::vector<float>& pars(){return P_;}           // cell-major: P_[c*nk+k]
  const std::vector<float>& pars()const{return P_;}
  const std::vector<double>& affinity()const{return A_;}
  void seed_patches(int count,int radius,double density);  // random blobs with random P
  void step();
 private:
  void affinity_();
  void transport_();
  Params p_;long t_=0;
  std::vector<std::int64_t> m_;std::vector<float> P_;
  std::vector<double> A_;std::vector<double> pbar_;
  std::vector<std::vector<double>> khat_re_,khat_im_;  // kernel spectra
};

// 2-D FFT on n*n complex data (n power of two), in place; inv divides by n*n.
void fft2(std::vector<double>& re,std::vector<double>& im,int n,bool inv);

}  // namespace oee::l0
