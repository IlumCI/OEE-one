// World v1 W0 checks (docs/WORLD.md): determinism, energy accounting, observation layout.
#include <algorithm>
#include <cstdio>
#include <vector>

#include "w/life.hpp"
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
    {WParams q7=q;q7.m_cost=4;q7.m_give=16;World w;w.init(q7,0);for(auto& r:w.res)r=0;const int c=5*L+5,f=5*L+6;put(w,c,"",10,1);
      for(int t2=0;t2<8;++t2){born.clear();w.tick(l1,born);}CHECK(w.mst[c]==8,"machine economy: m_cost 4 charges 1 store per 4 ticks of running");
      put(w,c,"C",40,1);const int chg=(w.t+1)%4==0;born.clear();w.tick(l1,born);
      CHECK(w.mat[f]==1&&w.mst[f]==16&&w.mst[c]==40-chg-16-16,"machine economy: C moves m_give from the parent into the copy");
      const int before=w.mst[f];born.clear();w.tick(l1,born);CHECK(w.mst[f]<=before&&w.mst[f]>0&&w.mhalt[f]==0,"machine economy: an endowed copy runs");
      std::FILE* fl=std::tmpfile();w.save(fl);std::rewind(fl);World v;const bool ok=v.load(fl);std::fclose(fl);
      CHECK(ok&&v.p.m_cost==4&&v.p.m_give==16&&v.mst==w.mst&&v.mat==w.mat,"machine economy: save/load keeps m_cost and m_give");}
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
    {auto gz=[&](int graze,int own,int ml){WParams q4=q;q4.graze=graze;q4.own=own;q4.K=2;World w;w.init(q4,0);for(auto& r:w.res)r=0;const int f=4*L+5;
        w.mat[f]=1;w.mst[f]=100;w.mlin[f]=(std::int8_t)ml;w.mbt[f]=-1;Agent& a=w.ag[0];a=Agent{};a.x=5;a.y=5;a.dir=0;a.energy=100;a.alive=true;a.id=1;w.occ[5*L+5]=0;
        std::vector<std::int8_t> la((std::size_t)q4.S*32,(std::int8_t)-100),lb((std::size_t)q4.S*32,(std::int8_t)-100);la[4]=100;const std::int8_t* l4[2]={la.data(),lb.data()};
        std::vector<int> bn;w.tick(l4,bn);return w.ag[0].energy-(100-q4.base);};
      CHECK(gz(1,0,0)==q.bite&&gz(0,0,0)==0&&gz(1,1,-1)==q.bite&&gz(1,1,1)==0&&gz(1,1,0)==q.bite,
        "grazing: eat takes from the machine in front (any without ownership; wild or own with it); not with graze 0");}
    {auto cp=[&](int own){WParams q8=q;q8.own=own;World w;w.init(q8,0);for(auto& r:w.res)r=0;const int c=5*L+5,f=5*L+6;put(w,c,"C",40,1);
        std::vector<int> bn;w.tick(l1,bn);return w.mat[f]==1?(int)w.mlin[f]:-9;};
      CHECK(cp(1)==0&&cp(2)==-1,"own 2: copies of an owned machine are wild; own 1: they keep the owner");}
    {auto ps=[&](int pass,int& ate){WParams q9=q;q9.pass=pass;q9.graze=1;q9.K=1;World w;w.init(q9,0);for(auto& r:w.res)r=0;const int f=4*L+5;
        w.mat[f]=1;w.mst[f]=100;w.mlin[f]=-1;w.mbt[f]=-1;Agent& a=w.ag[0];a=Agent{};a.x=5;a.y=5;a.dir=0;a.energy=100;a.alive=true;a.id=1;w.occ[5*L+5]=0;
        std::vector<std::int8_t> la((std::size_t)q9.S*32,(std::int8_t)-100);la[1]=100;const std::int8_t* l9[1]={la.data()};std::vector<int> bn;w.tick(l9,bn);
        const bool on=w.ag[0].y==4;std::fill(la.begin(),la.end(),(std::int8_t)-100);la[4]=100;const int e0=w.ag[0].energy;w.tick(l9,bn);ate=w.ag[0].energy-(e0-q9.base);return on;};
      int a1=0,a0=0;const bool on1=ps(1,a1),on0=ps(0,a0);
      CHECK(on1&&!on0&&a1==q.bite,"pass: agents walk onto machine cells and graze the machine underfoot; without pass machines block");}
    {WParams q10=q;q10.irr=2;q10.regrow=16;World w;w.init(q10,0);for(auto& r:w.res)r=0;for(auto& f:w.fert)f=0;const int c=5*L+5;put(w,c,",",100,0);
      std::vector<int> bn;w.tick(l1,bn);const bool up=w.irl[c]==2&&w.mst[c]==100-1-1;  // I (tape position 0) runs in the first tick: 1 store, plus 1 upkeep
      for(int t2=0;t2<300;++t2){bn.clear();w.tick(l1,bn);}const bool grew=w.irl[c]>10&&w.res[c]>10;  // the irrigated barren cell regrows (the machine has no E)
      World z;WParams q11=q10;q11.irr_decay=64;z.init(q11,0);z.irl[3]=10;for(int t2=0;t2<64;++t2){bn.clear();z.tick(l1,bn);}const bool dec=z.irl[3]==9;
      std::FILE* fl=std::tmpfile();w.save(fl);std::rewind(fl);World v;const bool ok=v.load(fl);std::fclose(fl);
      if(!(up&&grew&&dec))std::printf("  irl %d mst %d res %d dec %d\n",(int)w.irl[c],(int)w.mst[c],(int)w.res[c],(int)z.irl[3]);
      CHECK(up&&grew&&dec&&ok&&v.p.irr==2&&v.irl==w.irl,"W4a irrigation: I raises its cell's irrigation for 1 store, the cell regrows toward it, decays 1 per irr_decay ticks, saved");}
    {auto cl=[&](int claim){WParams q12=q;q12.own=1;q12.claim=claim;q12.K=2;World w;w.init(q12,0);for(auto& r:w.res)r=0;const int f=4*L+5;
        w.mat[f]=1;w.mst[f]=0;w.mlin[f]=-1;w.mbt[f]=-1;Agent& a=w.ag[q12.S];a=Agent{};a.x=5;a.y=5;a.dir=0;a.energy=100;a.lineage=1;a.alive=true;a.id=1;w.occ[5*L+5]=q12.S;
        std::vector<std::int8_t> la((std::size_t)q12.S*32,(std::int8_t)-100),lb((std::size_t)q12.S*32,(std::int8_t)-100);lb[9]=100;const std::int8_t* l12[2]={la.data(),lb.data()};
        std::vector<int> bn;w.tick(l12,bn);return (int)w.mlin[f];};
      CHECK(cl(1)==1&&cl(0)==-1,"W4b claim: feeding a wild machine makes it the feeder's lineage's; without claim it stays wild");}
    {WParams q13=q;q13.irr=2;q13.K=2;q13.pass=1;q13.graze=1;World w;w.init(q13,0);w.track=true;for(auto& r:w.res)r=0;const int f=4*L+5;put(w,f,",",0,0);w.mlin[f]=-1;
      Agent& a=w.ag[q13.S];a=Agent{};a.x=5;a.y=5;a.dir=0;a.energy=200;a.lineage=1;a.alive=true;a.id=1;w.occ[5*L+5]=q13.S;
      std::vector<std::int8_t> la((std::size_t)q13.S*32,(std::int8_t)-100),lb((std::size_t)q13.S*32,(std::int8_t)-100);lb[9]=100;const std::int8_t* l13[2]={la.data(),lb.data()};
      std::vector<int> bn;w.tick(l13,bn);w.tick(l13,bn);  // lineage 1 feeds the machine, which irrigates its own cell
      const bool cred=w.irl[f]>0&&w.ilin[f]==1&&w.stat[32+27]==2*q13.feed_amt;
      Agent& o=w.ag[0];o=Agent{};o.x=5;o.y=4;o.dir=0;o.energy=100;o.lineage=0;o.alive=true;o.id=2;w.occ[f]=0;w.res[f]=50;
      std::fill(lb.begin(),lb.end(),(std::int8_t)-100);lb[0]=100;la[4]=100;w.tick(l13,bn);  // lineage 0 eats on the cell that lineage 1's feeding irrigated
      const bool leak=w.stat[32+25]==q13.bite&&w.stat[28]==q13.bite&&w.stat[24]==0;
      CHECK(cred&&leak,"irrigation credit (analysis): a cell irrigated by a machine that lineage 1 fed is credited to 1; food eaten there by lineage 0 counts as 1's leak");}
    {WParams q14=q;q14.irr=2;q14.K=2;q14.pass=1;q14.graze=1;q14.land=1;World w;w.init(q14,0);for(auto& r:w.res)r=0;const int f=4*L+5;put(w,f,",",0,0);w.mlin[f]=-1;
      Agent& a=w.ag[q14.S];a=Agent{};a.x=5;a.y=5;a.dir=0;a.energy=200;a.lineage=1;a.alive=true;a.id=1;w.occ[5*L+5]=q14.S;
      std::vector<std::int8_t> la((std::size_t)q14.S*32,(std::int8_t)-100),lb((std::size_t)q14.S*32,(std::int8_t)-100);lb[9]=100;const std::int8_t* l14[2]={la.data(),lb.data()};
      std::vector<int> bn;w.tick(l14,bn);w.tick(l14,bn);const bool owned=w.irl[f]>0&&w.lown[f]==1&&w.mfd[f]==1;  // lineage 1 fed the machine that irrigated the cell
      w.mat[f]=0;w.mst[f]=0;w.res[f]=50;Agent& o=w.ag[0];o=Agent{};o.x=5;o.y=4;o.dir=0;o.energy=100;o.lineage=0;o.alive=true;o.id=2;w.occ[f]=0;
      std::vector<std::int8_t> ob(w.din()),ob1(w.din());w.observe(0,ob.data());w.observe(q14.S,ob1.data());
      const bool seen=ob[12]==0&&ob[122]==-64&&ob1[7]==25&&ob1[122]==0;  // 0 sees no food on 1's land; 1 sees it in front
      std::fill(lb.begin(),lb.end(),(std::int8_t)-100);lb[0]=100;la[4]=100;w.tick(l14,bn);const bool shut=w.res[f]==50&&w.ag[0].energy==100-q14.base;
      w.ag[0].alive=false;w.occ[f]=-1;w.ag[q14.S].y=4;w.occ[5*L+5]=-1;w.occ[f]=q14.S;
      std::fill(la.begin(),la.end(),(std::int8_t)-100);std::fill(lb.begin(),lb.end(),(std::int8_t)-100);lb[4]=100;w.tick(l14,bn);const bool fed=w.res[f]==50-q14.bite;
      std::FILE* fl=std::tmpfile();w.save(fl);std::rewind(fl);World v;const bool ok=v.load(fl);std::fclose(fl);const bool kept=ok&&v.p.land==1&&v.lown==w.lown&&v.mfd==w.mfd;
      w.irl[f]=1;for(int t2=0;t2<q14.irr_decay;++t2){bn.clear();w.tick(l14,bn);}const bool lapsed=w.irl[f]==0&&w.lown[f]==-1;
      if(!(owned&&seen&&shut&&fed&&kept&&lapsed))std::printf("  owned %d seen %d shut %d fed %d kept %d lapsed %d\n",owned,seen,shut,fed,kept,lapsed);
      CHECK(owned&&seen&&shut&&fed&&kept&&lapsed,"W4d land: a cell irrigated by a machine lineage 1 fed is 1's land; lineage 0 sees and eats no food there, 1 does; saved; lapses at irrigation 0");}
    {WParams q15=q;q15.irr=8;q15.irr_decay=1;q15.K=2;q15.pass=1;q15.graze=1;q15.land=1;q15.farms=6;q15.farm_store=40;q15.farm_every=50;World w;w.init(q15,0);w.track=true;for(auto& r:w.res)r=0;
      Agent& a=w.ag[q15.S];a=Agent{};a.x=1;a.y=1;a.dir=0;a.energy=500;a.lineage=1;a.alive=true;a.id=1;w.occ[1*L+1]=q15.S;
      std::vector<std::int8_t> la((std::size_t)q15.S*32,(std::int8_t)-100),lb((std::size_t)q15.S*32,(std::int8_t)-100);la[0]=100;lb[0]=100;const std::int8_t* l15[2]={la.data(),lb.data()};
      std::vector<int> bn;w.tick(l15,bn);bool placed=true,none0=true;
      for(int i=0;i<6;++i){const int c=w.home_cell(1,i);placed=placed&&w.mat[c]==1&&w.mlin[c]==1&&w.lown[c]==1&&w.irl[c]>200&&w.tape[(std::size_t)c*16]==',';}
      for(int i=0;i<6;++i)none0=none0&&w.lown[w.home_cell(0,i)]!=0;
      const int c=w.home_cell(1,0);w.occ[1*L+1]=-1;a.x=c%L;a.y=c/L;w.occ[c]=q15.S;const long h0=w.stat[32+29];bn.clear();w.tick(l15,bn);const bool home=w.stat[32+29]==h0+1;
      w.mst[c]=5;while(w.t%50!=0){bn.clear();w.tick(l15,bn);}bn.clear();w.tick(l15,bn);const bool topped=w.mst[c]>=38;
      std::FILE* fl=std::tmpfile();w.save(fl);std::rewind(fl);World v;const bool ok=v.load(fl);std::fclose(fl);const bool kept=ok&&v.p.farms==6&&v.p.farm_store==40&&v.p.farm_every==50;
      if(!(placed&&none0&&home&&topped&&kept))std::printf("  placed %d none0 %d home %d topped %d (mst %d) kept %d\n",placed,none0,home,topped,(int)w.mst[c],kept);
      CHECK(placed&&none0&&home&&topped&&kept,"W4e homesteads: farms on a living lineage's own land (none for an absent one), counted as home, topped up every farm_every ticks, saved");}
    {WParams q16=q;q16.irr=8;q16.irr_decay=1;q16.K=2;q16.pass=1;q16.graze=1;q16.land=1;q16.farms=6;q16.farm_store=0;q16.farm_every=50;q16.farm_reset=0;q16.farm_free=1;q16.irr_full=1;q16.m_decay=20;
      World w;w.init(q16,0);for(auto& r:w.res)r=0;Agent& a=w.ag[q16.S];a=Agent{};a.x=1;a.y=1;a.dir=0;a.energy=5000;a.lineage=1;a.alive=true;a.id=1;w.occ[1*L+1]=q16.S;
      std::vector<std::int8_t> la((std::size_t)q16.S*32,(std::int8_t)-100),lb((std::size_t)q16.S*32,(std::int8_t)-100);la[0]=100;lb[0]=100;const std::int8_t* l16[2]={la.data(),lb.data()};
      std::vector<int> bn;w.tick(l16,bn);const int c0=w.home_cell(1,0),c1=w.home_cell(1,1);const bool start=w.irl[c0]>250&&w.mst[c0]==0&&w.hfm[c0]==1&&w.lown[c0]==1;
      while(w.t<60){bn.clear();w.tick(l16,bn);}const bool stays=w.mat[c0]==1&&w.hfm[c0]==1&&w.mst[c0]==0&&w.irl[c0]<200;  // no refill, no re-irrigation, no decay
      w.mat[c1]=0;w.hfm[c1]=0;const int ir1=w.irl[c1];while(w.t<101){bn.clear();w.tick(l16,bn);}
      const bool rebuilt=w.mat[c1]==1&&w.mlin[c1]==1&&w.hfm[c1]==1&&w.mst[c1]==0&&w.irl[c1]<=ir1;
      std::FILE* fl=std::tmpfile();w.save(fl);std::rewind(fl);World v;const bool ok=v.load(fl);std::fclose(fl);const bool kept=ok&&v.p.farm_reset==0&&v.hfm==w.hfm;
      if(!(start&&stays&&rebuilt&&kept))std::printf("  start %d stays %d rebuilt %d kept %d\n",start,stays,rebuilt,kept);
      CHECK(start&&stays&&rebuilt&&kept,"W4f no free lunch: farms placed once on irrigated land with no store; never refilled or re-irrigated, never decay; a destroyed one is rebuilt empty; saved");}
    {WParams q17=q;q17.irr=8;q17.irr_decay=1;q17.K=2;q17.pass=1;q17.graze=1;q17.land=1;q17.farms=6;q17.farm_store=0;q17.farm_every=50;q17.farm_reset=0;q17.farm_free=1;q17.irr_full=1;q17.home_cue=1;
      World w;w.init(q17,0);for(auto& r:w.res)r=0;Agent& a=w.ag[q17.S];a=Agent{};a.x=1;a.y=1;a.dir=0;a.energy=5000;a.lineage=1;a.alive=true;a.id=1;w.occ[1*L+1]=q17.S;
      std::vector<std::int8_t> la((std::size_t)q17.S*32,(std::int8_t)-100),lb((std::size_t)q17.S*32,(std::int8_t)-100);la[0]=100;lb[0]=100;const std::int8_t* l17[2]={la.data(),lb.data()};
      std::vector<int> bn;w.tick(l17,bn);int bd=1<<30,ef=0,er=0,bc=-1;
      for(int i=0;i<6;++i){const int c=w.home_cell(1,i);int dx=c%L-1,dy=c/L-1;if(dx>L/2)dx-=L;if(dx<-L/2)dx+=L;if(dy>L/2)dy-=L;if(dy<-L/2)dy+=L;
        const int d=std::abs(dx)+std::abs(dy);if(d<bd){bd=d;bc=c;ef=-dy;er=dx;}}  // facing north: forward = -dy, right = +dx
      w.mst[bc]=60;w.irl[bc]=200;std::vector<std::int8_t> ob(w.din());w.observe(q17.S,ob.data());
      const bool cue=ob[123]==std::clamp(8*ef,-127,127)&&ob[124]==std::clamp(8*er,-127,127)&&ob[125]==30&&ob[126]==100;
      w.nocue=1u<<1;w.observe(q17.S,ob.data());const bool off=ob[123]==0&&ob[124]==0&&ob[125]==0&&ob[126]==0;w.nocue=0;
      std::FILE* fl=std::tmpfile();w.save(fl);std::rewind(fl);World v;const bool ok=v.load(fl);std::fclose(fl);const bool kept=ok&&v.p.home_cue==1&&v.p.farm_reset==0;
      if(!(cue&&off&&kept))std::printf("  cue %d (%d %d %d %d vs %d %d) off %d kept %d\n",cue,ob[123],ob[124],ob[125],ob[126],8*ef,8*er,off,kept);
      CHECK(cue&&off&&kept,"W4g cue: egocentric offset to the nearest own farm, its store and irrigation; zeros under the nocue ablation; saved");}
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
      const int n0=a.wipe(0);int left=0;for(int x=0;x<q.L*q.L;++x)left+=a.mat[x]==1&&a.mlin[x]==0;CHECK(n0>0&&left==0,"W3: wipe removes a lineage's machines");}
    {World a,b;a.init(q,5);b.init(q,5);b.probe=true;std::vector<std::int8_t> rl((std::size_t)q.S*32);
      for(int t=0;t<120;++t){for(std::size_t i=0;i<rl.size();++i)rl[i]=(std::int8_t)((i*41+t*13)%29-14);const std::int8_t* l[1]={rl.data()};born.clear();a.tick(l,born);born.clear();b.tick(l,born);}
      World w;w.init(q,0);w.probe=true;for(auto& r:w.res)r=0;auto pt=[&](int slot,int x,int y,int e){Agent& g=w.ag[slot];g=Agent{};g.x=x;g.y=y;g.dir=0;g.energy=e;g.alive=true;g.id=slot+1;w.occ[y*L+x]=slot;};
      pt(0,5,5,100);w.res[5*L+5]=50;pt(1,8,8,100);w.mat[7*L+8]=2;pt(2,11,11,1);  // 0 eats 32 of 50; 1 walks into a wall; 2 starves
      std::vector<std::int8_t> lp((std::size_t)q.S*32,(std::int8_t)-100);lp[4]=100;lp[32+1]=100;lp[64]=100;const std::int8_t* l2[1]={lp.data()};born.clear();w.tick(l2,born);
      CHECK(hash(a)==hash(b)&&w.pact[0]==4&&w.peff[0]==32&&w.ag[0].energy==100+32-q.base&&w.pact[1]==1&&w.peff[1]==0&&w.pdie[2]==1&&w.pdie[0]==0&&w.pact[3]==-1,
        "probe: recording leaves the world unchanged; actions, effects (bite, blocked move) and deaths recorded");
      auto gt=[&](int ge,int ga,int food){World v;v.init(q,0);v.probe=true;for(auto& r:v.res)r=0;Agent& g=v.ag[0];g=Agent{};g.x=5;g.y=5;g.dir=0;g.energy=100;g.alive=true;g.id=1;v.occ[5*L+5]=0;
        v.res[5*L+5]=(std::uint8_t)food;v.gate_eat=ge;v.gate_all=ga;std::vector<std::int8_t> lq((std::size_t)q.S*32,(std::int8_t)-128);lq[4]=100;lq[1]=127;lq[9]=110;
        const std::int8_t* l3[1]={lq.data()};born.clear();v.tick(l3,born);return (int)v.pact[0];};
      CHECK(gt(0,0,0)==1&&gt(1,0,0)==1&&gt(0,1,0)==1,"(setup) fwd has the top logit");
      auto gt2=[&](int ge,int ga,int food){World v;v.init(q,0);v.probe=true;for(auto& r:v.res)r=0;Agent& g=v.ag[0];g=Agent{};g.x=5;g.y=5;g.dir=0;g.energy=100;g.alive=true;g.id=1;v.occ[5*L+5]=0;
        v.res[5*L+5]=(std::uint8_t)food;v.gate_eat=ge;v.gate_all=ga;std::vector<std::int8_t> lq((std::size_t)q.S*32,(std::int8_t)-128);lq[4]=127;lq[1]=100;lq[9]=110;
        const std::int8_t* l3[1]={lq.data()};born.clear();v.tick(l3,born);return (int)v.pact[0];};
      CHECK(gt2(0,0,0)==4&&gt2(1,0,0)==9&&gt2(0,1,0)==1&&gt2(1,0,50)==4,"gates: an eat that would take nothing is drawn again (gate_eat: feed next; gate_all: feed is futile too, so fwd); with food it stays");}}
  {// lifetime learning: an action that costs energy every time is suppressed during life; one that pays is reinforced
    LifeCfg c;c.on=1;c.eta=10;Life lf;lf.init(1,8,7);std::int8_t h[8]={40,-30,20,10,-50,60,5,-15};const std::uint32_t m=c.mask(7,false);
    std::vector<std::int8_t> base(16,0);auto pa=[&](int a){std::vector<std::int8_t> l=base;lf.apply(c,m,0,h,l.data());const std::int64_t* E=act_weights();
      std::int64_t S=0;for(int k=0;k<7;++k)S+=E[(int)l[k]+128];return (double)E[(int)l[a]+128]/S;};
    const double f0=pa(1),e0=pa(4);
    for(int t=0;t<3000;++t){std::vector<std::int8_t> l=base;lf.apply(c,m,0,h,l.data());const int a=act_sample(l.data(),(std::uint64_t)t*2654435761ULL+7,7);
      const int dE=a==1?-3:a==4?2:-1;lf.learn(c,m,0,h,l.data(),a,dE);}
    const double f1=pa(1),e1=pa(4);char msg[160];std::snprintf(msg,sizeof msg,"lifetime learning: costly action %.3f -> %.3f, paying action %.3f -> %.3f; reproduction not plastic",f0,f1,e0,e1);
    CHECK(f1<f0/2&&e1>e0*2&&!((m>>5)&1u)&&((c.mask(7,false)>>4)&1u),msg);
    Life z;z.init(1,8,7);LifeCfg off=c;off.on=0;for(int t=0;t<50;++t){std::vector<std::int8_t> l=base;z.learn(off,m,0,h,l.data(),1,-3);}
    bool zero=true;for(auto w:z.W)zero=zero&&w==0;CHECK(zero,"lifetime learning off: synapses stay zero");}
  std::printf(fails?"%d FAILED\n":"all passed\n",fails);return fails?1:0;
}
