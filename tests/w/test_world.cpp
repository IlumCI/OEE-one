// World v1 W0 checks (docs/WORLD.md): determinism, energy accounting, observation layout.
#include <algorithm>
#include <cstdio>
#include <vector>

#include "w/world.hpp"

using namespace oee::w;
static int fails=0;
#define CHECK(c,msg) do{if(!(c)){std::printf("FAIL %s\n",msg);++fails;}else std::printf("ok   %s\n",msg);}while(0)

static std::uint64_t hash(const World& w){std::uint64_t h=1469598103934665603ULL;auto f=[&](std::uint64_t x){h=(h^x)*1099511628211ULL;};
  for(auto r:w.res){f(r);}
  for(auto& a:w.ag){f(a.alive);f(a.x);f(a.y);f(a.dir);f((std::uint64_t)(a.energy+1000000));f(a.age);}f(w.t);return h;}

int main(){
  WParams p;p.L=16;p.S=16;p.K=2;p.seed=7;p.nact=7;  // exercise attack too
  std::vector<std::int8_t> lg((std::size_t)p.S*World::kV);
  auto drive=[&](World& w,int ticks){std::vector<int> born;for(int t=0;t<ticks;++t){
      for(std::size_t i=0;i<lg.size();++i)lg[i]=(std::int8_t)((i*37+t*11)%23-11);  // fixed pseudo-random policy
      const std::int8_t* l[2]={lg.data(),lg.data()};born.clear();w.tick(l,born);}};
  {World a,b;a.init(p,6);b.init(p,6);drive(a,80);drive(b,80);CHECK(hash(a)==hash(b),"deterministic: same seed, same state after 80 ticks");
    World c;WParams q=p;q.seed=8;c.init(q,6);drive(c,80);CHECK(hash(a)!=hash(c),"different seed, different state");
    World d=a;drive(d,10);drive(a,10);CHECK(hash(a)==hash(d),"copies (forks) evolve identically");}
  {WParams q=p;q.regrow=0;q.base=1;q.move=0;q.turn=0;World w;w.init(q,6);
    std::vector<std::int8_t> eat((std::size_t)q.S*World::kV,0);for(int s=0;s<q.S;++s)eat[(std::size_t)s*World::kV+4]=100;  // everyone eats
    long e0=0,a0=0;for(auto& a:w.ag)if(a.alive){e0+=a.energy;++a0;}const long r0=w.resources();
    std::vector<int> born;const std::int8_t* l[2]={eat.data(),eat.data()};w.tick(l,born);
    long e1=0;for(auto& a:w.ag)if(a.alive)e1+=a.energy;const long r1=w.resources();
    CHECK(e1+r1==e0+r0-a0,"energy accounting: eaten resource becomes energy, minus base cost");}
  {World w;w.init(p,6);std::vector<std::int8_t> o(World::kDin);int slot=-1;for(int s=0;s<p.K*p.S;++s)if(w.ag[s].alive){slot=s;break;}
    w.observe(slot,o.data());const Agent& a=w.ag[slot];CHECK(o[12]==(std::int8_t)(w.res[a.y*p.L+a.x]>>1)&&o[52]==64,"observation: centre cell is own cell, bias set");
    w.observe(p.S-1+0*slot,o.data());bool z=!w.ag[p.S-1].alive;if(z){bool all=true;for(auto x:o)all=all&&x==0;CHECK(all,"observation: empty slot gives zeros");}}
  {World a;a.init(p,6);drive(a,40);std::FILE* f=std::tmpfile();a.save(f);std::rewind(f);World b;const bool ok=b.load(f);std::fclose(f);
    CHECK(ok&&hash(a)==hash(b),"save/load: same state");drive(a,30);drive(b,30);CHECK(hash(a)==hash(b),"save/load: same future");}
  // W2a: seasons, hidden resources, signals
  {WParams q;q.L=16;q.S=16;q.K=2;q.seed=5;q.nact=7;q.season=40;q.view=1;q.signals=1;q.sig_cost=1;
    std::vector<std::int8_t> l2((std::size_t)q.S*World::kV);
    auto drive2=[&](World& w,int ticks){std::vector<int> born;for(int t=0;t<ticks;++t){
        for(std::size_t i=0;i<l2.size();++i)l2[i]=(std::int8_t)((i*37+t*11)%23-11);const std::int8_t* l[2]={l2.data(),l2.data()};born.clear();w.tick(l,born);}};
    World a,b;a.init(q,6);b.init(q,6);drive2(a,60);drive2(b,60);CHECK(hash(a)==hash(b)&&a.din()==128,"W2a: deterministic, din 128 with signals");
    int ns=0;for(auto& x:a.ag)ns+=x.alive&&x.sig>0;CHECK(ns>0,"W2a: agents emit symbols");
    std::FILE* f=std::tmpfile();a.save(f);std::rewind(f);World c;const bool ok=c.load(f);std::fclose(f);
    bool sg=true;for(std::size_t i=0;i<a.ag.size();++i)sg=sg&&a.ag[i].sig==c.ag[i].sig;
    CHECK(ok&&hash(a)==hash(c)&&c.per==a.per&&c.ph==a.ph&&c.fp==a.fp&&sg&&c.p.season==40&&c.p.view==1,"W2a save/load: same state");
    drive2(a,40);drive2(c,40);CHECK(hash(a)==hash(c),"W2a save/load: same future");
    World d;d.init(q,0);const int N=q.L*q.L;int c0=0;for(int x=0;x<N;++x)if(d.fp[x]>d.fp[c0])c0=x;
    int lo=999,hi=-1;for(long t=0;t<d.per;++t){d.t=t;lo=std::min(lo,d.cap(c0));hi=std::max(hi,d.cap(c0));}
    char msg[120];std::snprintf(msg,sizeof msg,"W2a seasons: capacity at a patch centre ranges %d..%d over a period of %d",lo,hi,d.per);CHECK(hi>150&&lo<hi/3,msg);
    const int per0=d.per;auto ph0=d.ph;d.set_climate(7);CHECK(d.ph!=ph0||d.per!=per0,"W2a seasons: a new climate changes the phases");
    World e;e.init(q,0);e.t=0;auto place=[&](int slot,int x,int y,int dir,int sig){Agent& g=e.ag[slot];g=Agent{};g.x=x;g.y=y;g.dir=dir;g.energy=100;g.lineage=slot/q.S;g.alive=true;g.id=slot+1;g.sig=sig;e.occ[y*q.L+x]=slot;};
    place(0,5,5,0,3);place(q.S,5,4,0,2);e.res[3*q.L+5]=200;e.res[4*q.L+5]=100;  // facing north: (5,4) in front, (5,3) two ahead
    std::vector<std::int8_t> o(e.din());e.observe(0,o.data());
    CHECK(o[7]==50&&o[2]==0&&o[25+7]==-64&&o[53+7]==64&&o[78]==96,"W2a observation: hidden beyond view 1, neighbour symbol heard, own symbol");
    e.deaf=1;e.observe(0,o.data());CHECK(o[53+7]==0&&o[78]==96&&o[25+7]==-64,"W2a observation: deaf hears no symbols, still sees the agent");}
  std::printf(fails?"%d FAILED\n":"all passed\n",fails);return fails?1:0;
}
