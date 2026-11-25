// World v1 (docs/WORLD.md), phase W0: one lineage evolved by integer EGGROLL in mirrored forks of a persistent world;
// plus ancestor tournaments.
// Train:   oee_w -epochs N [-pairs 128] [-H 200] [-L 32] [-S 64] [-founders 16] [-layers 2] [-D 64] [-sig 4] [-seed 1] [-regrow 4] [-base 1] [-patches 6]
//                [-threads 4] [-alpha_floor 0] [-archive DIR] [-archive_every 50] [-log FILE]
//          Log: epoch canon_pop canon_biomass fork_fit_mean fork_fit_sd changed alpha sec reseeds canon_resources  (fork fitness = mean biomass over the episode)
// Tourney: oee_w -tourney A.egg B.egg [-seeds 5] [-ticks 2000] [-founders 16] [-L 32] [-S 64] [-seed0 1000]
//          Two lineages in identical fresh arenas (A = lineage 0, B = lineage 1); prints per-seed biomass share of A.
// Epoch: copy the canonical world into 2P forks; fork 2p uses +noise, 2p+1 the same noise negated (same world seed:
// common random numbers); fitness = lineage biomass summed over the H ticks; F_p = sign(fit_2p - fit_2p+1); integer EGGROLL
// update; then the canonical world advances H ticks with the updated mean.
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include "evo/egg.hpp"
#include "w/world.hpp"

using namespace oee;

namespace {
// Run H ticks for worlds ws (one lineage, rows w*S+s) with stepper st; resets states of newborns.
// integ (optional): per world, lineage-0 biomass summed over the ticks (dense fitness: rewards lasting longer even
// when every agent eventually dies).
void run_worlds(std::vector<w::World>& ws,evo::Rows& rows,evo::Stepper& st,int S,int H,std::vector<long>* integ=nullptr){
  const int R=(int)ws.size()*S;std::vector<std::int8_t> obs((std::size_t)R*w::World::kDin);std::vector<int> born;
  const int L=(int)(rows.state.size()/rows.R);
  for(int t=0;t<H;++t){
    for(std::size_t i=0;i<ws.size();++i)for(int s=0;s<S;++s)ws[i].observe(s,obs.data()+((std::size_t)i*S+s)*w::World::kDin);
    const std::int8_t* out=st.step(obs.data());
    for(std::size_t i=0;i<ws.size();++i){const std::int8_t* lp=out+(std::size_t)i*S*w::World::kV;born.clear();ws[i].tick(&lp,born);
      for(int b:born)std::fill(rows.state.begin()+((std::size_t)i*S+b)*L,rows.state.begin()+((std::size_t)i*S+b+1)*L,0);
      if(integ)(*integ)[i]+=ws[i].biomass(0);}
  }
}
evo::EggCfg brain_cfg(int layers,int D,int sig){evo::EggCfg c;c.L=layers;c.D=D;c.sig=sig;c.V=w::World::kV;c.Din=w::World::kDin;return c;}
}  // namespace

int main(int argc,char** argv){
  int epochs=100,P=128,H=200,threads=4,layers=2,D=64,sig=4,archive_every=50,seeds=5,ticks=2000;double alpha_floor=0;
  std::uint64_t seed=1,seed0=1000;std::string logf,archive,ta,tb;w::WParams wp;int founders=16;
  for(int a=1;a<argc;a+=2){const char* s=argv[a];const char* v=a+1<argc?argv[a+1]:"";
    if(!std::strcmp(s,"-epochs"))epochs=std::atoi(v);else if(!std::strcmp(s,"-pairs"))P=std::atoi(v);else if(!std::strcmp(s,"-H"))H=std::atoi(v);
    else if(!std::strcmp(s,"-L"))wp.L=std::atoi(v);else if(!std::strcmp(s,"-S"))wp.S=std::atoi(v);else if(!std::strcmp(s,"-founders"))founders=std::atoi(v);
    else if(!std::strcmp(s,"-layers"))layers=std::atoi(v);else if(!std::strcmp(s,"-D"))D=std::atoi(v);else if(!std::strcmp(s,"-sig"))sig=std::atoi(v);
    else if(!std::strcmp(s,"-seed"))seed=std::strtoull(v,nullptr,10);else if(!std::strcmp(s,"-threads"))threads=std::atoi(v);
    else if(!std::strcmp(s,"-alpha_floor"))alpha_floor=std::atof(v);else if(!std::strcmp(s,"-archive"))archive=v;
    else if(!std::strcmp(s,"-archive_every"))archive_every=std::atoi(v);else if(!std::strcmp(s,"-log"))logf=v;
    else if(!std::strcmp(s,"-seeds"))seeds=std::atoi(v);else if(!std::strcmp(s,"-ticks"))ticks=std::atoi(v);else if(!std::strcmp(s,"-seed0"))seed0=std::strtoull(v,nullptr,10);
    else if(!std::strcmp(s,"-regrow"))wp.regrow=std::atoi(v);else if(!std::strcmp(s,"-base"))wp.base=std::atoi(v);else if(!std::strcmp(s,"-patches"))wp.patches=std::atoi(v);
    else if(!std::strcmp(s,"-tourney")&&a+2<argc){ta=argv[a+1];tb=argv[a+2];++a;}
    else{std::fprintf(stderr,"unknown option %s\n",s);return 2;}}
  if(wp.S%16){std::fprintf(stderr,"-S must be a multiple of 16\n");return 2;}
  const evo::EggCfg bc=brain_cfg(layers,D,sig);

  if(!ta.empty()){  // tournament: A = lineage 0, B = lineage 1
    evo::Egg A(bc,1),B(bc,2);A.load(ta);B.load(tb);double sum=0;
    for(int k=0;k<seeds;++k){w::WParams q=wp;q.K=2;q.seed=seed0+k;w::World W;W.init(q,founders);
      evo::Rows rows;rows.init(A,2*q.S);evo::Stepper sa(A,nullptr,rows,0,q.S),sb(B,nullptr,rows,q.S,q.S);
      std::vector<std::int8_t> obs((std::size_t)2*q.S*w::World::kDin);std::vector<int> born;const int Ls=(int)(rows.state.size()/rows.R);
      for(int t=0;t<ticks;++t){for(int s=0;s<2*q.S;++s)W.observe(s,obs.data()+(std::size_t)s*w::World::kDin);
        const std::int8_t* oa=sa.step(obs.data());const std::int8_t* ob=sb.step(obs.data()+(std::size_t)q.S*w::World::kDin);
        const std::int8_t* lp[2]={oa,ob};born.clear();W.tick(lp,born);
        for(int b:born)std::fill(rows.state.begin()+(std::size_t)b*Ls,rows.state.begin()+(std::size_t)(b+1)*Ls,0);}
      const long ba=W.biomass(0),bb=W.biomass(1);const double sh=ba+bb>0?(double)ba/(ba+bb):0.5;sum+=sh;
      std::printf("seed %llu\tshareA %.4f\tbiomass %ld %ld\tcount %d %d\n",(unsigned long long)q.seed,sh,ba,bb,W.count(0),W.count(1));}
    std::printf("mean_shareA %.4f\n",sum/seeds);return 0;
  }

  evo::Egg g(bc,seed);evo::Noise nz;wp.K=1;wp.seed=seed;w::World canon;canon.init(wp,founders);
  evo::Rows rc;rc.init(g,wp.S);
  std::FILE* lg=logf.empty()?stdout:std::fopen(logf.c_str(),"w");if(!lg)return 2;
  std::fprintf(lg,"# W0 L%d S%d founders %d H%d pairs %d brain L%d D%d sig%d params %ld threads %d seed %llu regrow %d base %d patches %d\n",wp.L,wp.S,founders,H,P,layers,D,sig,g.nparams(),threads,(unsigned long long)seed,wp.regrow,wp.base,wp.patches);
  if(!archive.empty())g.save(archive+"/egg_0.bin",0);
  int reseeds=0;
  for(int e=0;e<epochs;++e){
    const auto t0=std::chrono::steady_clock::now();
    nz.gen(g,P,seed,e);const int F2=2*P;std::vector<long> fit(F2,0);
    const int T=std::max(1,std::min(threads,F2));std::vector<std::thread> th;
    for(int ti=0;ti<T;++ti){const int f0=F2*ti/T,f1=F2*(ti+1)/T;
      th.emplace_back([&,f0,f1]{const int nw=f1-f0;std::vector<w::World> ws(nw,canon);evo::Rows r;r.init(g,nw*wp.S);
        const std::size_t st=rc.state.size();
        for(int i=0;i<nw;++i){for(int s=0;s<wp.S;++s){const int row=i*wp.S+s;r.pair[row]=(f0+i)/2;r.sign[row]=((f0+i)%2==0)?1:-1;}
          std::copy(rc.state.begin(),rc.state.end(),r.state.begin()+(std::size_t)i*st);}
        evo::Stepper sp(g,&nz,r,0,nw*wp.S);std::vector<long> integ(nw,0);run_worlds(ws,r,sp,wp.S,H,&integ);
        for(int i=0;i<nw;++i)fit[f0+i]=integ[i]/H;});}
    for(auto& x:th)x.join();
    std::vector<int> F(P);double m=0,m2=0;for(int p=0;p<P;++p){F[p]=fit[2*p]>fit[2*p+1]?1:fit[2*p]<fit[2*p+1]?-1:0;}
    for(long f:fit){m+=f;m2+=(double)f*f;}m/=F2;const double sd=std::sqrt(std::max(0.0,m2/F2-m*m));
    const double alpha=std::max(alpha_floor,1.0/(0.015*e+1));const long ch=evo::egg_update(g,nz,F,alpha);
    {std::vector<w::World> one{canon};evo::Stepper sc(g,nullptr,rc,0,wp.S);run_worlds(one,rc,sc,wp.S,H);canon=one[0];}
    if(canon.count(0)==0){canon.reseed_lineage(0,founders,(std::uint64_t)e+1);std::fill(rc.state.begin(),rc.state.end(),0);++reseeds;}
    const double sec=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
    std::fprintf(lg,"%d\t%d\t%ld\t%.1f\t%.1f\t%ld\t%.4f\t%.2f\t%d\t%ld\n",e+1,canon.count(0),canon.biomass(0),m,sd,ch,alpha,sec,reseeds,canon.resources());std::fflush(lg);
    if(!archive.empty()&&((e+1)%archive_every==0||e+1==epochs))g.save(archive+"/egg_"+std::to_string(e+1)+".bin",e+1);
  }
  if(lg!=stdout)std::fclose(lg);
}
