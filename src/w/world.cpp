#include "w/world.hpp"

#include <algorithm>
#include <cmath>

namespace oee::w {

namespace {
std::uint64_t mix(std::uint64_t x){x+=0x9e3779b97f4a7c15ULL;x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;x=(x^(x>>27))*0x94d049bb133111ebULL;return x^(x>>31);}
const int FX[4]={0,1,0,-1},FY[4]={-1,0,1,0};  // dir 0 = north (y-1), clockwise
}  // namespace

int act_of(const std::int8_t* l){int b=0;for(int a=1;a<World::kActions;++a)if(l[a]>l[b])b=a;return b;}
int act_sample(const std::int8_t* l,std::uint64_t bits){
  static std::int64_t E[256];static bool init=[]{for(int i=0;i<256;++i)E[i]=(std::int64_t)std::llround(16*std::exp2(i/16.0));return true;}();(void)init;
  std::int64_t w[World::kActions],tot=0;for(int a=0;a<World::kActions;++a){w[a]=E[(int)l[a]+128];tot+=w[a];}
  std::int64_t u=(std::int64_t)(bits%(std::uint64_t)tot);for(int a=0;a<World::kActions;++a){if(u<w[a])return a;u-=w[a];}return World::kActions-1;
}

void World::init(const WParams& pp,int founders){
  p=pp;t=0;next_id=1;const int N=p.L*p.L;res.assign(N,0);fert.assign(N,0);racc.assign(N,0);occ.assign(N,-1);ag.assign((std::size_t)p.K*p.S,Agent{});
  std::vector<double> f(N,0);
  for(int i=0;i<p.patches;++i){const std::uint64_t h=mix(p.seed*0x51ed27ULL+i);const double cx=(h&0xffff)/65536.0*p.L,cy=((h>>16)&0xffff)/65536.0*p.L;
    const double r=3+((h>>32)&0xff)/255.0*3;
    for(int y=0;y<p.L;++y)for(int x=0;x<p.L;++x){double dx=std::fabs(x-cx),dy=std::fabs(y-cy);dx=std::min(dx,p.L-dx);dy=std::min(dy,p.L-dy);
      f[y*p.L+x]+=255*std::exp(-(dx*dx+dy*dy)/(2*r*r));}}
  for(int c=0;c<N;++c){fert[c]=(std::uint8_t)std::min(255.0,std::floor(f[c]));res[c]=fert[c];}
  for(int k=0;k<p.K;++k)reseed_lineage(k,founders,0);
}

void World::reseed_lineage(int k,int founders,std::uint64_t salt){
  const int N=p.L*p.L;int placed=0;
  for(std::uint64_t i=0;placed<founders&&i<(std::uint64_t)64*N;++i){
    const std::uint64_t h=mix(mix(p.seed^0xf00dULL)+((std::uint64_t)k<<40)+(salt<<20)+i);const int c=(int)(h%N);if(occ[c]>=0)continue;
    if(fert[c]<32&&i<(std::uint64_t)32*N)continue;  // prefer fertile cells
    int s=-1;for(int j=0;j<p.S;++j)if(!ag[(std::size_t)k*p.S+j].alive){s=j;break;}if(s<0)break;
    Agent& a=ag[(std::size_t)k*p.S+s];a=Agent{};a.x=c%p.L;a.y=c/p.L;a.dir=(int)((h>>32)&3);a.energy=p.e0;a.lineage=k;a.alive=true;a.id=next_id++;
    occ[c]=k*p.S+s;++placed;}
}

void World::observe(int slot,std::int8_t* o) const{
  std::fill(o,o+kDin,0);const Agent& a=ag[slot];if(!a.alive)return;
  const int fx=FX[a.dir],fy=FY[a.dir],rx=-fy,ry=fx;int i=0;  // right = forward rotated clockwise
  for(int v=2;v>=-2;--v)for(int u=-2;u<=2;++u,++i){
    const int x=((a.x+v*fx+u*rx)%p.L+p.L)%p.L,y=((a.y+v*fy+u*ry)%p.L+p.L)%p.L;const int c=y*p.L+x;
    o[i]=(std::int8_t)(res[c]>>1);const int q=occ[c];
    if(q>=0&&q!=slot)o[25+i]=(std::int8_t)(ag[q].lineage==a.lineage?64:-64);}
  o[50]=(std::int8_t)std::min(127,a.energy/8);o[51]=(std::int8_t)std::min(127,a.age/4);o[52]=64;
}

void World::tick(const std::int8_t* const* logits,std::vector<int>& born){
  const int N=p.L*p.L;std::vector<int> order;
  for(int s=0;s<p.K*p.S;++s)if(ag[s].alive)order.push_back(s);
  for(int i=(int)order.size()-1;i>0;--i){const int j=(int)(mix(mix(p.seed^0x7e1cULL)+((std::uint64_t)t<<20)+i)%(std::uint64_t)(i+1));std::swap(order[i],order[j]);}
  std::vector<int> act(p.K*p.S,0);
  for(int s:order){const int k=s/p.S;const std::int8_t* l=logits[k]+(std::size_t)(s%p.S)*kV;
    act[s]=p.stoch?act_sample(l,mix(mix(p.seed^0xac7ULL)+((std::uint64_t)t<<24)+ag[s].id)):act_of(l);}  // same draws in mirrored forks
  for(int s:order){Agent& a=ag[s];if(!a.alive)continue;const int c=a.y*p.L+a.x;
    const int fc=((a.y+FY[a.dir]+p.L)%p.L)*p.L+(a.x+FX[a.dir]+p.L)%p.L;
    switch(act[s]){
      case 1:if(occ[fc]<0){occ[c]=-1;a.x=fc%p.L;a.y=fc/p.L;occ[fc]=s;}a.energy-=p.move;break;
      case 2:a.dir=(a.dir+3)&3;a.energy-=p.turn;break;
      case 3:a.dir=(a.dir+1)&3;a.energy-=p.turn;break;
      case 4:{const int take=std::min<int>(res[c],p.bite);res[c]-=(std::uint8_t)take;a.energy=std::min(p.ecap,a.energy+take);break;}
      case 5:if(a.energy>=p.repro_min&&occ[fc]<0){const int k=a.lineage;int j=-1;
          for(int q=0;q<p.S;++q)if(!ag[(std::size_t)k*p.S+q].alive){j=q;break;}
          if(j>=0){const int cs=k*p.S+j;Agent& b=ag[cs];b=Agent{};b.x=fc%p.L;b.y=fc/p.L;b.dir=a.dir;b.energy=p.repro_give;b.lineage=k;b.alive=true;b.id=next_id++;
            occ[fc]=cs;a.energy-=p.repro_give+p.repro_cost;born.push_back(cs);}}
        break;
      default:break;}
  }
  for(int s=0;s<p.K*p.S;++s){Agent& a=ag[s];if(!a.alive)continue;a.energy-=p.base;++a.age;
    if(a.energy<=0||a.age>p.max_age){a.alive=false;occ[a.y*p.L+a.x]=-1;}}
  if(p.regrow>0)for(int c=0;c<N;++c)if(res[c]<fert[c]){int a=racc[c]+fert[c]*p.regrow;int r=res[c];while(a>=4096&&r<fert[c]){++r;a-=4096;}
      racc[c]=(std::uint16_t)std::min(a,4095);res[c]=(std::uint8_t)r;}  // rate fert*regrow/4096 per tick; regrow 0: none
  ++t;
}

long World::biomass(int k) const{long b=0;for(int s=0;s<p.S;++s){const Agent& a=ag[(std::size_t)k*p.S+s];if(a.alive)b+=a.energy;}return b;}
int World::count(int k) const{int n=0;for(int s=0;s<p.S;++s)n+=ag[(std::size_t)k*p.S+s].alive;return n;}
long World::resources() const{long r=0;for(auto x:res)r+=x;return r;}

}  // namespace oee::w
