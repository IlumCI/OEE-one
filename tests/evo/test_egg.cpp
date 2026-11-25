// EGG / integer EGGROLL checks (docs/EVO.md).
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "evo/egg.hpp"

using namespace oee::evo;
static int fails=0;
#define CHECK(c,msg) do{if(!(c)){std::printf("FAIL %s\n",msg);++fails;}else std::printf("ok   %s\n",msg);}while(0)

int main(){
  {bool ok=true;for(int m=1;m<128&&ok;++m)for(int x=-65535;x<=65535;++x){int q=x/m;if((x%m!=0)&&(x<0))--q;if(egg_fdiv(x,m)!=q){ok=false;break;}}
    CHECK(ok,"layer norm floor division exact for all |x| < 2^16, 1 <= m <= 127");}
  EggCfg c;c.L=2;c.D=64;
  {Egg a(c,1),b(c,1),d(c,2);bool same=true,diff=false;for(std::size_t k=0;k<a.mats.size();++k){same=same&&a.mats[k]->w==b.mats[k]->w;diff=diff||a.mats[k]->w!=d.mats[k]->w;}
    CHECK(same&&diff,"init: same seed same weights, other seed differs");
    double s1=0,s2=0;long n=0;for(auto* m:a.mats)for(auto x:m->w){s1+=x;s2+=x*x;++n;}
    const double sd=std::sqrt(s2/n);char msg[96];std::snprintf(msg,sizeof msg,"init: weight sd %.2f (target 16)",sd);CHECK(sd>15&&sd<17,msg);}
  // forward: determinism across thread counts; noise off and sign 0 identical; antithetic +/- differ
  Egg g(c,3);const int R=64,T=12;Noise nz;nz.gen(g,R/2,9,0);
  std::vector<std::uint8_t> tok((std::size_t)R*(T+1));for(std::size_t i=0;i<tok.size();++i)tok[i]=(std::uint8_t)(37*i+11);
  for(int r=0;r<R;++r)std::memcpy(tok.data()+(std::size_t)r*(T+1),tok.data()+(std::size_t)(r/2)*(T+1),T+1);  // pairs share data
  auto run=[&](int th,bool noise,int signmode){Rows rr;rr.init(g,R);for(int r=0;r<R;++r){rr.pair[r]=r/2;rr.sign[r]=signmode?(r%2?-1:1):0;}
    std::vector<std::int64_t> f(R,0);run_rows(g,noise?&nz:nullptr,rr,tok.data(),T,f.data(),th);return f;};
  auto f1=run(1,true,1),f3=run(3,true,1);CHECK(f1==f3,"forward: identical with 1 and 3 threads");
  auto z0=run(2,false,1),z1=run(2,true,0);CHECK(z0==z1,"forward: no noise == sign 0");
  bool pm=false;for(int p=0;p<R/2;++p)pm=pm||f1[2*p]!=f1[2*p+1];CHECK(pm,"forward: antithetic members differ");
  bool same=true;for(int p=0;p<R/2;++p)same=same&&z0[2*p]==z0[2*p+1];CHECK(same,"forward: unperturbed pair members identical");
  // update: alpha small -> few changes; F = 0 -> no change; determinism
  {Egg a(c,5);Noise n2;n2.gen(a,32,1,0);std::vector<int> F(32,0);const long ch0=egg_update(a,n2,F,0.5);
    for(int p=0;p<32;++p){F[p]=p%3==0?1:-1;}
    const long ch1=egg_update(a,n2,F,0.05),tot=a.nparams();
    char msg[120];std::snprintf(msg,sizeof msg,"update: F=0 changes %ld; alpha=0.05 changes %.3f of params (expect ~0.05)",ch0,(double)ch1/tot);
    CHECK(ch0==0&&ch1>0&&ch1<0.12*tot,msg);}
  std::printf(fails?"%d FAILED\n":"all passed\n",fails);return fails?1:0;
}
