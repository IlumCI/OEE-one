#include "w/life.hpp"

#include <algorithm>

#include "w/world.hpp"

namespace oee::w {

std::uint32_t LifeCfg::mask(int na,bool matter) const{std::uint32_t m=0;
  for(int a=0;a<na&&a<32;++a){if(a==5&&!repro)continue;if(matter&&a>=7&&a<=10)continue;if(!matter&&a==7)continue;m|=1u<<a;}return m;}
void Life::init(int RR,int DD,int nn){R=RR;D=DD;na=nn;J=D+1;W.assign((std::size_t)R*na*J,0);e.assign((std::size_t)R*na*J,0);b.assign(R,0);}
void Life::reset(int r){const std::size_t n=(std::size_t)na*J;std::fill(W.begin()+r*n,W.begin()+(r+1)*n,0);std::fill(e.begin()+r*n,e.begin()+(r+1)*n,0);b[r]=0;}
void Life::inherit(int rd,int rs){const std::size_t n=(std::size_t)na*J;for(std::size_t i=0;i<n;++i){W[rd*n+i]=W[rs*n+i]>>1;e[rd*n+i]=0;}b[rd]=b[rs];}
void Life::copy_row(int rd,const Life& s,int rs){const std::size_t n=(std::size_t)na*J;
  std::copy(s.W.begin()+rs*n,s.W.begin()+(rs+1)*n,W.begin()+rd*n);std::copy(s.e.begin()+rs*n,s.e.begin()+(rs+1)*n,e.begin()+rd*n);b[rd]=s.b[rs];}
namespace {
inline int xin(const std::int8_t* h,int j,int D){return j<D?h[j]:64;}
}
void Life::apply(const LifeCfg& c,std::uint32_t msk,int r,const std::int8_t* h,std::int8_t* l) const{
  if(!c.on)return;
  for(int a=0;a<na;++a){if(!((msk>>a)&1u))continue;const std::int32_t* w=&W[((std::size_t)r*na+a)*J];std::int64_t s=0;
    for(int j=0;j<J;++j)s+=(std::int64_t)w[j]*xin(h,j,D);
    l[a]=(std::int8_t)std::clamp<std::int64_t>((std::int64_t)l[a]+(s>>16),-128,127);}}
// Units: g = 1[a chosen] - pi_a in Q15; trace e += g*x*256 per tick (x the input, int8); delta = dE - baseline in Q8
// (energy units * 256); m in Q7 (128 = 1). dW = delta*e*m >> (eta + 15), i.e. W += 2^-eta * m * delta(energy) * e.
// One tick with g = 1, delta = 1 energy unit, m = 1 changes the action's plastic logit by about 2^-eta * 256 * |x|^2 /
// 65536 (|x|^2 summed over the J inputs).
void Life::learn(const LifeCfg& c,std::uint32_t msk,int r,const std::int8_t* h,const std::int8_t* l,int a,int dE){
  if(!c.on)return;
  const std::int64_t* E=act_weights();std::int64_t S=0;for(int k=0;k<na;++k)S+=E[(int)l[k]+128];if(S<=0)return;
  const std::int64_t d=(std::int64_t)dE*256-b[r];b[r]+=(std::int32_t)(d>>c.beta);
  const std::int64_t m=c.mod?(std::int64_t)l[c.mlog]+128:128;
  for(int k=0;k<na;++k){if(!((msk>>k)&1u))continue;const std::int64_t g=((k==a?S:0)-E[(int)l[k]+128])*32768/S;
    std::int32_t* w=&W[((std::size_t)r*na+k)*J];std::int32_t* t=&e[((std::size_t)r*na+k)*J];
    for(int j=0;j<J;++j){const std::int64_t tr=((std::int64_t)t[j]*c.lam>>8)+((g*xin(h,j,D))>>7);t[j]=(std::int32_t)std::clamp<std::int64_t>(tr,-(1LL<<30),1LL<<30);
      const std::int64_t nw=(std::int64_t)w[j]+((d*t[j]*m)>>(c.eta+15));w[j]=(std::int32_t)std::clamp<std::int64_t>(nw,-(1LL<<28),1LL<<28);}}}
void Life::learn_obs(const LifeCfg& c,std::uint32_t msk,int r,const std::int8_t* h,const std::int8_t* l,int a,std::int64_t d){
  if(!c.on||a<0||a>=na||!((msk>>a)&1u))return;
  const std::int64_t* E=act_weights();std::int64_t S=0;for(int k=0;k<na;++k)S+=E[(int)l[k]+128];if(S<=0)return;
  const std::int64_t m=c.mod?(std::int64_t)l[c.mlog]+128:128;
  for(int k=0;k<na;++k){if(!((msk>>k)&1u))continue;const std::int64_t g=((k==a?S:0)-E[(int)l[k]+128])*32768/S;std::int32_t* w=&W[((std::size_t)r*na+k)*J];
    for(int j=0;j<J;++j){const std::int64_t nw=(std::int64_t)w[j]+((d*((g*xin(h,j,D))>>7)*m)>>(c.eta+16));w[j]=(std::int32_t)std::clamp<std::int64_t>(nw,-(1LL<<28),1LL<<28);}}}
void Life::save(std::FILE* f) const{const std::int32_t hd[3]={R,D,na};std::fwrite(hd,4,3,f);std::fwrite(W.data(),4,W.size(),f);std::fwrite(e.data(),4,e.size(),f);std::fwrite(b.data(),4,b.size(),f);}
bool Life::load(std::FILE* f){std::int32_t hd[3];if(std::fread(hd,4,3,f)!=3||hd[0]!=R||hd[1]!=D||hd[2]!=na)return false;
  return std::fread(W.data(),4,W.size(),f)==W.size()&&std::fread(e.data(),4,e.size(),f)==e.size()&&std::fread(b.data(),4,b.size(),f)==b.size();}

}  // namespace oee::w
