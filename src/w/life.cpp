#include "w/life.hpp"

#include <algorithm>

#include "w/world.hpp"

namespace oee::w {

std::uint32_t LifeCfg::mask(int na,bool matter) const{std::uint32_t m=0;
  for(int a=0;a<na&&a<32;++a){if(a==5&&!repro)continue;if(matter&&a>=7&&a<=10)continue;if(!matter&&a==7)continue;m|=1u<<a;}return m;}
void Life::init(int RR,int DD,int nn,int ns,int xx,int oo){R=RR;D=DD;na=nn;nl=na+ns;nx=xx;so=oo;J=D+1+nx;W.assign((std::size_t)R*nl*J,0);e.assign((std::size_t)R*nl*J,0);b.assign(R,0);}
void Life::reset(int r){const std::size_t n=(std::size_t)nl*J;std::fill(W.begin()+r*n,W.begin()+(r+1)*n,0);std::fill(e.begin()+r*n,e.begin()+(r+1)*n,0);b[r]=0;}
void Life::inherit(int rd,int rs,int keep){const std::size_t n=(std::size_t)nl*J;
  for(std::size_t i=0;i<n;++i){const std::int32_t w=W[rs*n+i];W[rd*n+i]=keep<=1?w>>1:w-(w>>keep);e[rd*n+i]=0;}b[rd]=b[rs];}
void Life::copy_row(int rd,const Life& s,int rs){const std::size_t n=(std::size_t)nl*J;
  std::copy(s.W.begin()+rs*n,s.W.begin()+(rs+1)*n,W.begin()+rd*n);std::copy(s.e.begin()+rs*n,s.e.begin()+(rs+1)*n,e.begin()+rd*n);b[rd]=s.b[rs];}
namespace {
inline int xin(const std::int8_t* h,const std::int8_t* x,int j,int D){return j<D?h[j]:j==D?64:x[j-D-1];}
}
void Life::apply(const LifeCfg& c,std::uint32_t msk,int r,const std::int8_t* h,const std::int8_t* x,std::int8_t* l) const{
  if(!c.on)return;
  for(int a=0;a<nl;++a){if(a<na&&!((msk>>a)&1u))continue;const std::int32_t* w=&W[((std::size_t)r*nl+a)*J];std::int64_t s=0;
    for(int j=a<na||c.symh?0:D;j<J;++j)s+=(std::int64_t)w[j]*xin(h,x,j,D);
    const int li=a<na?a:so+a-na;l[li]=(std::int8_t)std::clamp<std::int64_t>((std::int64_t)l[li]+(s>>16),-128,127);}}
// Units: g = 1[a chosen] - pi_a in Q15; trace e += g*x*256 per tick (x the input, int8); delta = dE - baseline in Q8
// (energy units * 256); m in Q7 (128 = 1). dW = delta*e*m >> (eta + 15), i.e. W += 2^-eta * m * delta(energy) * e.
// One tick with g = 1, delta = 1 energy unit, m = 1 changes the action's plastic logit by about 2^-eta * 256 * |x|^2 /
// 65536 (|x|^2 summed over the J inputs). The symbol head (rows na..na+3) is a second softmax over logits so..so+3 with
// its own g (the symbol emitted) and the same delta, traces and rate.
void Life::learn8(const LifeCfg& c,std::uint32_t msk,int r,const std::int8_t* h,const std::int8_t* x,const std::int8_t* l,int a,int sg,std::int64_t r8){
  if(!c.on)return;
  const std::int64_t* E=act_weights();std::int64_t S=0;for(int k=0;k<na;++k)S+=E[(int)l[k]+128];if(S<=0)return;
  const std::int64_t d=r8-b[r];b[r]+=(std::int32_t)(d>>c.beta);
  const std::int64_t m=c.mod?(std::int64_t)l[c.mlog]+128:128;
  std::int64_t Ss=0;if(nl>na)for(int k=0;k<4;++k)Ss+=E[(int)l[so+k]+128];
  for(int k=0;k<nl;++k){std::int64_t g;
    if(k<na){if(!((msk>>k)&1u))continue;g=((k==a?S:0)-E[(int)l[k]+128])*32768/S;}
    else g=Ss>0&&sg>=0?((k-na==sg?Ss:0)-E[(int)l[so+k-na]+128])*32768/Ss:0;
    std::int32_t* w=&W[((std::size_t)r*nl+k)*J];std::int32_t* t=&e[((std::size_t)r*nl+k)*J];const int sh=(k<na||c.seta<0?c.eta:c.seta)+15,shx=c.xeta>=0?c.xeta+15:sh;
    for(int j=k<na||c.symh?0:D;j<J;++j){const std::int64_t tr=((std::int64_t)t[j]*c.lam>>8)+((g*xin(h,x,j,D))>>7);t[j]=(std::int32_t)std::clamp<std::int64_t>(tr,-(1LL<<30),1LL<<30);
      const std::int64_t nw=(std::int64_t)w[j]+((d*t[j]*m)>>(j>D?shx:sh));w[j]=(std::int32_t)std::clamp<std::int64_t>(nw,-(1LL<<28),1LL<<28);}}}
void Life::learn_obs(const LifeCfg& c,std::uint32_t msk,int r,const std::int8_t* h,const std::int8_t* x,const std::int8_t* l,int a,std::int64_t d){
  if(!c.on||a<0||a>=na||!((msk>>a)&1u))return;
  const std::int64_t* E=act_weights();std::int64_t S=0;for(int k=0;k<na;++k)S+=E[(int)l[k]+128];if(S<=0)return;
  const std::int64_t m=c.mod?(std::int64_t)l[c.mlog]+128:128;
  for(int k=0;k<na;++k){if(!((msk>>k)&1u))continue;const std::int64_t g=((k==a?S:0)-E[(int)l[k]+128])*32768/S;std::int32_t* w=&W[((std::size_t)r*nl+k)*J];
    for(int j=0;j<J;++j){const std::int64_t nw=(std::int64_t)w[j]+((d*((g*xin(h,x,j,D))>>7)*m)>>(c.eta+16));w[j]=(std::int32_t)std::clamp<std::int64_t>(nw,-(1LL<<28),1LL<<28);}}}
void Life::save(std::FILE* f) const{const std::int32_t hd[5]={R,D,na,nl,J};std::fwrite(hd,4,nl==na&&nx==0?3:5,f);  // the LL1 layout keeps its 3-word header
  std::fwrite(W.data(),4,W.size(),f);std::fwrite(e.data(),4,e.size(),f);std::fwrite(b.data(),4,b.size(),f);}
bool Life::load(std::FILE* f){std::int32_t hd[5];const std::size_t nh=nl==na&&nx==0?3:5;if(std::fread(hd,4,nh,f)!=nh||hd[0]!=R||hd[1]!=D||hd[2]!=na||(nh==5&&(hd[3]!=nl||hd[4]!=J)))return false;
  return std::fread(W.data(),4,W.size(),f)==W.size()&&std::fread(e.data(),4,e.size(),f)==e.size()&&std::fread(b.data(),4,b.size(),f)==b.size();}

}  // namespace oee::w
