// World v1 W0 checks (docs/WORLD.md): determinism, energy accounting, observation layout.
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
  WParams p;p.L=16;p.S=16;p.K=2;p.seed=7;
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
  std::printf(fails?"%d FAILED\n":"all passed\n",fails);return fails?1:0;
}
