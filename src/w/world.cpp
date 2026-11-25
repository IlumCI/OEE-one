#include "w/world.hpp"

#include <algorithm>
#include <cmath>

namespace oee::w {

namespace {
std::uint64_t mix(std::uint64_t x){x+=0x9e3779b97f4a7c15ULL;x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;x=(x^(x>>27))*0x94d049bb133111ebULL;return x^(x>>31);}
const int FX[4]={0,1,0,-1},FY[4]={-1,0,1,0};  // dir 0 = north (y-1), clockwise
}  // namespace

int act_of(const std::int8_t* l,int n){int b=0;for(int a=1;a<n;++a)if(l[a]>l[b])b=a;return b;}
int act_sample(const std::int8_t* l,std::uint64_t bits,int n){
  static std::int64_t E[256];static bool init=[]{for(int i=0;i<256;++i)E[i]=(std::int64_t)std::llround(16*std::exp2(i/16.0));return true;}();(void)init;
  std::int64_t w[World::kActions],tot=0;for(int a=0;a<n;++a){w[a]=E[(int)l[a]+128];tot+=w[a];}
  std::int64_t u=(std::int64_t)(bits%(std::uint64_t)tot);for(int a=0;a<n;++a){if(u<w[a])return a;u-=w[a];}return n-1;
}

void World::init(const WParams& pp,int founders){
  p=pp;t=0;next_id=1;const int N=p.L*p.L;res.assign(N,0);fert.assign(N,0);racc.assign(N,0);occ.assign(N,-1);ag.assign((std::size_t)p.K*p.S,Agent{});
  std::vector<double> f(N,0);
  for(int i=0;i<p.patches;++i){const std::uint64_t h=mix(p.seed*0x51ed27ULL+i);const double cx=(h&0xffff)/65536.0*p.L,cy=((h>>16)&0xffff)/65536.0*p.L;
    const double r=3+((h>>32)&0xff)/255.0*3;
    for(int y=0;y<p.L;++y)for(int x=0;x<p.L;++x){double dx=std::fabs(x-cx),dy=std::fabs(y-cy);dx=std::min(dx,p.L-dx);dy=std::min(dy,p.L-dy);
      f[y*p.L+x]+=255*std::exp(-(dx*dx+dy*dy)/(2*r*r));}}
  for(int c=0;c<N;++c){fert[c]=(std::uint8_t)std::min(255.0,std::floor(f[c]));res[c]=fert[c];}
  fp.clear();ph.clear();per=0;
  if(p.season>0){fp.assign((std::size_t)p.patches*N,0);  // the same bumps, one layer per patch
    for(int i=0;i<p.patches;++i){const std::uint64_t h=mix(p.seed*0x51ed27ULL+i);const double cx=(h&0xffff)/65536.0*p.L,cy=((h>>16)&0xffff)/65536.0*p.L;
      const double r=3+((h>>32)&0xff)/255.0*3;
      for(int y=0;y<p.L;++y)for(int x=0;x<p.L;++x){double dx=std::fabs(x-cx),dy=std::fabs(y-cy);dx=std::min(dx,p.L-dx);dy=std::min(dy,p.L-dy);
        fp[(std::size_t)i*N+y*p.L+x]=(std::uint8_t)std::min(255.0,std::floor(255*std::exp(-(dx*dx+dy*dy)/(2*r*r))));}}
    set_climate(0);for(int c=0;c<N;++c)res[c]=(std::uint8_t)cap(c);}
  for(int k=0;k<p.K;++k)reseed_lineage(k,founders,0);
}

void World::set_climate(std::uint64_t salt){
  if(p.season<=0)return;const std::uint64_t h=mix(mix(p.seed^0xc11eULL)+salt);
  per=std::max(4,p.season/2+(int)(h%(std::uint64_t)std::max(1,p.season)));ph.assign(p.patches,0);
  for(int i=0;i<p.patches;++i)ph[i]=(int)(mix(h+i+1)%(std::uint64_t)per);
}
namespace {
// productivity of a patch at phase x of period q: triangle wave, 256 at mid-cycle, 0 for the first and last quarter
inline int prod(int x,int q){const int a=std::abs(2*x-q);return std::max(0,256*(q-2*a)/q);}
}  // namespace
int World::cap(int c) const{
  if(p.season<=0)return fert[c];const int N=p.L*p.L;int s=0;
  for(int i=0;i<p.patches;++i)s+=fp[(std::size_t)i*N+c]*prod((int)((t+ph[i])%per),per);
  return std::min(255,s>>8);
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
  std::fill(o,o+din(),0);const Agent& a=ag[slot];if(!a.alive)return;
  const int fx=FX[a.dir],fy=FY[a.dir],rx=-fy,ry=fx;int i=0;  // right = forward rotated clockwise
  const bool hear=p.signals&&!((deaf>>a.lineage)&1);
  for(int v=2;v>=-2;--v)for(int u=-2;u<=2;++u,++i){
    const int x=((a.x+v*fx+u*rx)%p.L+p.L)%p.L,y=((a.y+v*fy+u*ry)%p.L+p.L)%p.L;const int c=y*p.L+x;
    if(std::max(std::abs(u),std::abs(v))<=p.view)o[i]=(std::int8_t)(res[c]>>1);const int q=occ[c];
    if(q>=0&&q!=slot){o[25+i]=(std::int8_t)(ag[q].lineage==a.lineage?64:-64);if(hear)o[53+i]=(std::int8_t)(32*ag[q].sig);}}
  o[50]=(std::int8_t)std::min(127,a.energy/8);o[51]=(std::int8_t)std::min(127,a.age/4);o[52]=64;
  if(p.signals)o[78]=(std::int8_t)(32*a.sig);
}

void World::tick(const std::int8_t* const* logits,std::vector<int>& born){
  const int N=p.L*p.L;std::vector<int> order;
  for(int s=0;s<p.K*p.S;++s)if(ag[s].alive)order.push_back(s);
  for(int i=(int)order.size()-1;i>0;--i){const int j=(int)(mix(mix(p.seed^0x7e1cULL)+((std::uint64_t)t<<20)+i)%(std::uint64_t)(i+1));std::swap(order[i],order[j]);}
  std::vector<int> act(p.K*p.S,0);
  for(int s:order){const int k=s/p.S;const std::int8_t* l=logits[k]+(std::size_t)(s%p.S)*kV;
    act[s]=p.stoch?act_sample(l,mix(mix(p.seed^0xac7ULL)+((std::uint64_t)t<<24)+ag[s].id),p.nact):act_of(l,p.nact);  // same draws in mirrored forks
    if(p.signals)ag[s].sig=act_sample(l+8,mix(mix(p.seed^0x5197ULL)+((std::uint64_t)t<<24)+ag[s].id),4);}  // heard next tick
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
      case 6:{const int q=occ[fc];a.energy-=p.atk_cost;
        if(q>=0&&ag[q].alive){Agent& v=ag[q];const int take=std::min(v.energy,p.atk_take);v.energy-=take;a.energy=std::min(p.ecap,a.energy+take);
          if(v.energy<=0){v.alive=false;occ[fc]=-1;}}
        break;}
      default:break;}
  }
  for(int s=0;s<p.K*p.S;++s){Agent& a=ag[s];if(!a.alive)continue;a.energy-=p.base+(a.sig>0?p.sig_cost:0);++a.age;
    if(a.energy<=0||a.age>p.max_age){a.alive=false;occ[a.y*p.L+a.x]=-1;}}
  if(p.season>0){int w[64];for(int i=0;i<p.patches&&i<64;++i)w[i]=prod((int)((t+ph[i])%per),per);  // seasonal capacity, rot above it
    for(int c=0;c<N;++c){int cp=0;for(int i=0;i<p.patches&&i<64;++i)cp+=fp[(std::size_t)i*N+c]*w[i];cp=std::min(255,cp>>8);
      if(res[c]>cp){res[c]=(std::uint8_t)std::max(cp,(int)res[c]-p.rot);continue;}
      if(p.regrow>0&&res[c]<cp){int a=racc[c]+cp*p.regrow;int r=res[c];while(a>=4096&&r<cp){++r;a-=4096;}racc[c]=(std::uint16_t)std::min(a,4095);res[c]=(std::uint8_t)r;}}}
  else if(p.regrow>0)for(int c=0;c<N;++c)if(res[c]<fert[c]){int a=racc[c]+fert[c]*p.regrow;int r=res[c];while(a>=4096&&r<fert[c]){++r;a-=4096;}
      racc[c]=(std::uint16_t)std::min(a,4095);res[c]=(std::uint8_t)r;}  // rate fert*regrow/4096 per tick; regrow 0: none
  ++t;
}

long World::biomass(int k) const{long b=0;for(int s=0;s<p.S;++s){const Agent& a=ag[(std::size_t)k*p.S+s];if(a.alive)b+=a.energy;}return b;}
int World::count(int k) const{int n=0;for(int s=0;s<p.S;++s)n+=ag[(std::size_t)k*p.S+s].alive;return n;}
long World::resources() const{long r=0;for(auto x:res)r+=x;return r;}
namespace {
void wr(std::FILE* f,std::int64_t x){std::fwrite(&x,8,1,f);}
bool rd(std::FILE* f,std::int64_t& x){return std::fread(&x,8,1,f)==1;}
constexpr std::int64_t kExt=0x5458453257LL;  // "W2EXT": marks the W2a extension of a world record
template<class T> void wv(std::FILE* f,const std::vector<T>& v){wr(f,(std::int64_t)v.size());if(!v.empty())std::fwrite(v.data(),sizeof(T),v.size(),f);}
template<class T> bool rv(std::FILE* f,std::vector<T>& v){std::int64_t n=0;if(!rd(f,n)||n<0||n>(1<<26))return false;v.resize((std::size_t)n);
  return n==0||std::fread(v.data(),sizeof(T),(std::size_t)n,f)==(std::size_t)n;}
}  // namespace

void World::save(std::FILE* f) const{
  const std::int64_t q[]={p.L,p.S,p.K,p.patches,p.e0,p.ecap,p.base,p.move,p.turn,p.bite,p.repro_min,p.repro_give,p.repro_cost,p.max_age,p.regrow,
    p.stoch,p.nact,p.atk_cost,p.atk_take,(std::int64_t)p.seed,t,(std::int64_t)next_id};
  for(auto x:q){wr(f,x);}
  if(ext()){wr(f,kExt);const std::int64_t e[]={p.season,p.view,p.signals,p.rot,p.sig_cost,per};for(auto x:e){wr(f,x);}wv(f,ph);wv(f,fp);}
  wv(f,res);wv(f,fert);wv(f,racc);wv(f,occ);wr(f,(std::int64_t)ag.size());
  for(auto& a:ag){const std::int64_t r[]={a.x,a.y,a.dir,a.energy,a.age,a.lineage,a.alive,(std::int64_t)a.id};for(auto x:r)wr(f,x);}
  if(ext())for(auto& a:ag){wr(f,a.sig);}
}
bool World::load(std::FILE* f){
  std::int64_t q[22];for(auto& x:q)if(!rd(f,x))return false;
  int* ip[]={&p.L,&p.S,&p.K,&p.patches,&p.e0,&p.ecap,&p.base,&p.move,&p.turn,&p.bite,&p.repro_min,&p.repro_give,&p.repro_cost,&p.max_age,&p.regrow,
    &p.stoch,&p.nact,&p.atk_cost,&p.atk_take};
  for(int i=0;i<19;++i){*ip[i]=(int)q[i];}p.seed=(std::uint64_t)q[19];t=q[20];next_id=(std::uint64_t)q[21];
  p.season=0;p.view=2;p.signals=0;p.rot=1;p.sig_cost=0;per=0;ph.clear();fp.clear();
  {std::int64_t m=0;const long pos=std::ftell(f);if(!rd(f,m))return false;
    if(m==kExt){std::int64_t e[6];for(auto& x:e)if(!rd(f,x))return false;p.season=(int)e[0];p.view=(int)e[1];p.signals=(int)e[2];p.rot=(int)e[3];p.sig_cost=(int)e[4];per=(int)e[5];
      if(!rv(f,ph)||!rv(f,fp))return false;}
    else if(std::fseek(f,pos,SEEK_SET)!=0)return false;}  // W1 record: no extension
  if(!rv(f,res)||!rv(f,fert)||!rv(f,racc)||!rv(f,occ)){return false;}std::int64_t n=0;if(!rd(f,n)||n<0||n>(1<<24)){return false;}ag.assign((std::size_t)n,Agent{});
  for(auto& a:ag){std::int64_t r[8];for(auto& x:r)if(!rd(f,x))return false;
    a.x=(int)r[0];a.y=(int)r[1];a.dir=(int)r[2];a.energy=(int)r[3];a.age=(int)r[4];a.lineage=(int)r[5];a.alive=r[6]!=0;a.id=(std::uint64_t)r[7];a.sig=0;}
  if(ext())for(auto& a:ag){std::int64_t x=0;if(!rd(f,x))return false;a.sig=(int)x;}
  return (int)res.size()==p.L*p.L&&res.size()==fert.size()&&res.size()==racc.size()&&res.size()==occ.size()&&(int)ag.size()==p.K*p.S;
}

void World::move_slot(int from,int to){Agent& a=ag[from];if(!a.alive)return;ag[to]=a;ag[to].lineage=to/p.S;occ[a.y*p.L+a.x]=to;a=Agent{};}

}  // namespace oee::w
