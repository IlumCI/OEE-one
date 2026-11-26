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
  // W3: machines and the agent actions that build and program them
  {WParams q;q.L=16;q.S=16;q.K=1;q.seed=9;q.nact=10;q.matter=1;q.regrow=0;q.stoch=0;const int L=q.L;
    auto blank=[&](World& w){w.init(q,0);for(auto& r:w.res)r=0;};
    auto put=[&](World& w,int c,const char* code,int store,int dir,int h0=0){w.mat[c]=1;for(int j=0;j<16;++j)w.tape[(std::size_t)c*16+j]=0;
      for(int j=0;code[j];++j)w.tape[(std::size_t)c*16+j]=(std::uint8_t)code[j];w.mst[c]=(std::int16_t)store;w.mdir[c]=(std::uint8_t)dir;w.mip[c]=0;w.mh0[c]=(std::uint8_t)h0;w.mh1[c]=0;
      w.mbt[c]=-1;w.mlin[c]=0;};
    std::vector<std::int8_t> nol((std::size_t)q.S*32,0);const std::int8_t* l1[1]={nol.data()};std::vector<int> born;
    {World w;blank(w);const int c=5*L+5;w.res[c]=100;put(w,c,"E",50,0);w.tick(l1,born);
      CHECK(w.mst[c]==57&&w.res[c]==92,"W3 machine: E harvests 8 from its cell, one store per tick of running");}
    {World w;blank(w);const int c=5*L+5,f=4*L+5;put(w,c,"D",20,0);w.tick(l1,born);CHECK(w.res[f]==8&&w.mst[c]==11,"W3 machine: D delivers 8 to the cell in front (north)");}
    {World w;blank(w);const int c=5*L+5,f=5*L+6;put(w,c,"C",40,1);w.tick(l1,born);bool same=true;for(int j=0;j<16;++j)same=same&&w.tape[(std::size_t)f*16+j]==w.tape[(std::size_t)c*16+j];
      CHECK(w.mat[f]==1&&same&&w.mst[c]==40-1-16&&w.mfx[(std::size_t)c*4+2]==1,"W3 machine: C copies its tape into the empty cell in front (east)");
      const int before=w.mst[f];w.tick(l1,born);CHECK(w.mst[f]==before,"W3 machine: a copy has no store, so it does not run");}
    {World w;blank(w);const int c=5*L+5,f=6*L+5;put(w,c,"W",40,2);w.tick(l1,born);CHECK(w.mat[f]==2,"W3 machine: W builds a wall in front (south)");}
    {World w;blank(w);const int c=5*L+5;w.res[c]=100;put(w,c,"[]E",50,0,0);w.tick(l1,born);  // tape[h0] = '[' (non-zero): ] loops back, E never runs
      World v;blank(v);v.res[c]=100;put(v,c,"[]E",50,0,5);v.tick(l1,born);                   // tape[5] = 0: [ jumps past ], E runs
      CHECK(w.res[c]==100&&v.res[c]==92,"W3 machine: [ and ] loop like BFF (skip when zero, repeat when non-zero)");}
    {World w;blank(w);const int c=5*L+5;put(w,c,"E",0,0);for(int i=0;i<=q.m_decay;++i)w.tick(l1,born);CHECK(w.mat[c]==0,"W3 machine: an empty store halts it, and it decays after m_decay ticks");}
    {World w;blank(w);Agent& a=w.ag[0];a=Agent{};a.x=5;a.y=5;a.dir=0;a.energy=300;a.alive=true;a.id=1;w.occ[5*L+5]=0;const int f=4*L+5;
      auto act=[&](int ac,int cd){std::fill(nol.begin(),nol.end(),(std::int8_t)-100);nol[ac]=100;nol[16+cd]=100;w.tick(l1,born);};
      act(8,0);const bool made=w.mat[f]==1&&w.mlin[f]==0;act(7,10);act(7,13);const bool wrote=w.tape[(std::size_t)f*16]=='E'&&w.tape[(std::size_t)f*16+1]=='C'&&w.ag[0].wpos==2;
      act(9,0);const bool fed=w.mst[f]>0;const int e0=w.ag[0].energy,st0=w.mst[f];act(8,0);const bool un=w.mat[f]==0&&w.ag[0].energy>=e0+st0-2;
      act(8,0);act(1,0);const bool blocked=w.ag[0].y==5;
      CHECK(made&&wrote&&fed&&un&&blocked,"W3 agent: make, write E and C, feed, unmake (store returned), machines block movement");}
    {WParams q2=q;q2.own=1;q2.K=2;World w;w.init(q2,0);for(auto& r:w.res)r=0;const int f=4*L+5;w.mat[f]=1;w.mst[f]=100;w.mlin[f]=0;w.mbt[f]=-1;
      Agent& a=w.ag[0];a=Agent{};a.x=5;a.y=5;a.dir=0;a.energy=100;a.alive=true;a.id=1;w.occ[5*L+5]=0;
      std::vector<std::int8_t> la((std::size_t)q2.S*32,(std::int8_t)-100),lb((std::size_t)q2.S*32,(std::int8_t)-100);la[4]=100;
      const std::int8_t* l2[2]={la.data(),lb.data()};w.tick(l2,born);const bool gran=w.mst[f]==100-q2.bite-1&&w.ag[0].energy==100+q2.bite-q2.base;  // -1: the machine runs this tick
      Agent& b=w.ag[q2.S];b=Agent{};b.x=5;b.y=3;b.dir=2;b.energy=100;b.lineage=1;b.alive=true;b.id=2;w.occ[3*L+5]=q2.S;  // lineage 1, facing the machine from the north
      std::fill(la.begin(),la.end(),(std::int8_t)-100);la[0]=100;lb[8]=100;const int eb=w.ag[q2.S].energy;w.tick(l2,born);
      CHECK(gran&&w.mat[f]==0&&w.ag[q2.S].energy==eb-2*q2.mk_cost-q2.base,"W3.2 ownership: owner eats from its granary; another lineage can only destroy it");}
    {WParams q3=q;q3.nodes=1;q3.node_per=50;q3.node_ripe=10;q3.view=1;World w;w.init(q3,0);for(auto& r:w.res)r=0;
      int nc=-1;for(int c=0;c<L*L;++c)if(w.mat[c]==3){nc=c;break;}w.mat[nc]=0;const int c=5*L+5;w.mat[c]=3;w.nph[c]=0;w.ncyc[c]=-1;  // one node at (5,5), ripe at t 0-9
      auto put2=[&](int slot,int x,int y,int dir){Agent& g=w.ag[slot];g=Agent{};g.x=x;g.y=y;g.dir=dir;g.energy=100;g.alive=true;g.id=slot+1;w.occ[y*L+x]=slot;};
      put2(0,5,6,0);put2(1,5,3,2);  // agent 0 south of the node facing it; agent 1 two cells north (facing south, not adjacent)
      std::vector<std::int8_t> o(w.din());w.observe(0,o.data());const bool near=o[7]==127&&o[79+7]==96;w.observe(1,o.data());const bool far=o[2]==0&&o[79+2]==96;
      std::vector<std::int8_t> le((std::size_t)q3.S*32,(std::int8_t)-100);for(int s=0;s<q3.S;++s)le[(std::size_t)s*32+4]=100;const std::int8_t* l3[1]={le.data()};
      w.tick(l3,born);const bool alone=w.ag[0].energy==100-q3.base&&w.ripe(c);  // one eater: nothing, still ripe
      w.ag[1].y=4;w.occ[3*L+5]=-1;w.occ[4*L+5]=1;w.tick(l3,born);const bool joint=w.ag[0].energy==99+q3.node_pay-q3.base&&w.ag[1].energy==99+q3.node_pay-q3.base&&!w.ripe(c);
      CHECK(near&&far&&alone&&joint,"W3.3 nodes: ripeness seen only nearby; one eater gets nothing; two eaters share the payout, node spent");}
    {WParams q4=q;q4.K=2;q4.signals=1;q4.kin_sig=1;World w;w.init(q4,0);auto put4=[&](int slot,int x,int y,int sig){Agent& g=w.ag[slot];g=Agent{};g.x=x;g.y=y;g.dir=0;g.energy=100;
        g.lineage=slot/q4.S;g.alive=true;g.id=slot+1;g.sig=sig;w.occ[y*L+x]=slot;};
      put4(0,5,5,0);put4(1,5,4,2);put4(q4.S,6,5,3);std::vector<std::int8_t> o(w.din());w.observe(0,o.data());  // kin in front (5,4), other lineage to the right (6,5)
      CHECK(o[53+7]==64&&o[53+13]==0&&o[25+13]==-64,"wave 2 kin_sig: own lineage's symbol heard, other lineage's not (still seen)");
      const int n=w.catastrophe(3,0.1);int out=0;for(auto& a:w.ag)out+=a.alive;CHECK(n+out==3,"wave 2 catastrophe: agents outside the window die");
      World v;WParams q5=q;q5.K=1;v.init(q5,0);Agent& m=v.ag[0];m=Agent{};m.x=5;m.y=5;m.dir=0;m.energy=500;m.alive=true;m.id=1;v.occ[5*L+5]=0;
      std::vector<std::int8_t> lr((std::size_t)q5.S*32,(std::int8_t)-100);lr[5]=100;const std::int8_t* l5[1]={lr.data()};born.clear();v.tick(l5,born);
      CHECK(born.size()==1&&v.parents.size()==1&&v.parents[0]==0,"wave 2: parents records the parent slot of each newborn");
      World z;WParams q6=q;q6.wall_decay=4096;z.init(q6,0);int w0=0;for(int c=0;c<L*L;c+=3){z.mat[c]=2;++w0;}std::vector<std::int8_t> lz((std::size_t)q6.S*32,0);
      const std::int8_t* l6[1]={lz.data()};for(int t2=0;t2<16;++t2){born.clear();z.tick(l6,born);}int w1=0;for(auto x:z.mat)w1+=x==2;
      CHECK(w1<w0&&w1>0,"walls erode at wall_decay/65536 per tick");}
    {World a,b;a.init(q,5);b.init(q,5);std::vector<std::int8_t> rl((std::size_t)q.S*32);auto drive3=[&](World& w,int n){for(int t=0;t<n;++t){
        for(std::size_t i=0;i<rl.size();++i)rl[i]=(std::int8_t)((i*41+t*13)%29-14);const std::int8_t* l[1]={rl.data()};born.clear();w.tick(l,born);}};
      drive3(a,120);drive3(b,120);int nm=0;for(auto x:a.mat)nm+=x==1;CHECK(hash(a)==hash(b)&&a.tape==b.tape&&nm>0,"W3: deterministic with agents building machines");
      std::FILE* f=std::tmpfile();a.save(f);std::rewind(f);World c;const bool ok=c.load(f);std::fclose(f);
      CHECK(ok&&c.mat==a.mat&&c.tape==a.tape&&c.mst==a.mst&&c.mfx==a.mfx&&c.p.matter==1,"W3 save/load: same matter state");
      drive3(a,50);drive3(c,50);CHECK(hash(a)==hash(c)&&a.tape==c.tape,"W3 save/load: same future");
      const int n0=a.wipe(0);int left=0;for(int x=0;x<q.L*q.L;++x)left+=a.mat[x]==1&&a.mlin[x]==0;CHECK(n0>0&&left==0,"W3: wipe removes a lineage's machines");}}
  std::printf(fails?"%d FAILED\n":"all passed\n",fails);return fails?1:0;
}
