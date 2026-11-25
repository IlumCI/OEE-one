// World v1 (docs/WORLD.md): lineages evolved by integer EGGROLL in mirrored forks of a persistent world;
// plus ancestor tournaments.
// Train:   oee_w -epochs N [-pairs 128] [-H 200] [-L 32] [-S 64] [-founders 16] [-layers 2] [-D 64] [-sig 4] [-seed 1]
//                [-regrow 4] [-base 1] [-patches 6] [-threads 4] [-alpha_floor 0] [-archive DIR] [-archive_every 50]
//                [-lineages 1] [-split_at 3S/4] [-nact 6] [-hist_frac 0] [-hist_founders 8] [-log FILE]
//          W0 = -lineages 1 -nact 6 -hist_frac 0 (bit-identical to the W0 binary).
//          W1 = -lineages K -nact 7 (attack) -hist_frac 4 (ancestor in every 4th fork pair).
//          Log: epoch canon_pop canon_biomass fork_fit_mean fork_fit_sd changed alpha sec reseeds canon_resources
//               [W1: active splits extinct lineage_id:count,...] (fork fitness = mean lineage biomass over the episode)
// Tourney: oee_w -tourney A.egg B.egg [-seeds 5] [-ticks 2000] [-founders 16] [-L 32] [-S 64] [-seed0 1000] [-nact 6]
//          Two lineages in identical fresh arenas (A = lineage 0, B = lineage 1); prints per-seed biomass share of A.
// Epoch: copy the canonical world into 2P forks; fork 2p uses +noise, 2p+1 the same noise negated (same world seed:
// common random numbers); each lineage has its own noise; fitness of lineage k = its biomass summed over the H ticks;
// F_kp = sign(fit_k,2p - fit_k,2p+1); integer EGGROLL update per lineage; the canonical world then advances H ticks.
// Lineage dynamics (W1): a lineage with no agents left goes extinct (archived); a lineage at >= split_at agents splits
// (copy of its mean with 10% of weights moved one step; half of its agents move to the new lineage).
// Historical self-play (W1): every hist_frac-th fork pair also contains hist_founders agents of one archived ancestor
// mean (no noise, not updated), as in Digital Red Queen (arXiv:2601.03335).
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "evo/egg.hpp"
#include "w/world.hpp"

using namespace oee;

namespace {
std::uint64_t mixu(std::uint64_t x){x+=0x9e3779b97f4a7c15ULL;x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;x=(x^(x>>27))*0x94d049bb133111ebULL;return x^(x>>31);}
// Run H ticks. Rows layout: row(k,i,s) = (k*nw + i)*S + s; st[k] null for lineages without agents in these worlds.
// integ[k][i] (optional): lineage-k biomass summed over the ticks in world i.
void run_worlds(std::vector<w::World>& ws,evo::Rows& rows,const std::vector<evo::Stepper*>& st,int S,int H,std::vector<std::vector<long>>* integ){
  const int nw=(int)ws.size(),K=(int)st.size(),Din=w::World::kDin;
  std::vector<std::int8_t> obs((std::size_t)K*nw*S*Din);std::vector<int> born;const int L=(int)(rows.state.size()/rows.R);
  const std::vector<std::int8_t> zero((std::size_t)S*w::World::kV,0);std::vector<const std::int8_t*> outs(K,nullptr),lp(K);
  for(int t=0;t<H;++t){
    for(int k=0;k<K;++k){if(!st[k])continue;for(int i=0;i<nw;++i)for(int s=0;s<S;++s)ws[i].observe(k*S+s,obs.data()+((std::size_t)(k*nw+i)*S+s)*Din);}
    for(int k=0;k<K;++k)outs[k]=st[k]?st[k]->step(obs.data()+(std::size_t)k*nw*S*Din):nullptr;
    for(int i=0;i<nw;++i){for(int k=0;k<K;++k)lp[k]=outs[k]?outs[k]+(std::size_t)i*S*w::World::kV:zero.data();born.clear();ws[i].tick(lp.data(),born);
      for(int b:born){const int k=b/S,s=b%S;std::fill(rows.state.begin()+((std::size_t)(k*nw+i)*S+s)*L,rows.state.begin()+((std::size_t)(k*nw+i)*S+s+1)*L,0);}
      if(integ)for(int k=0;k<K;++k)if(st[k])(*integ)[k][i]+=ws[i].biomass(k);}
  }
}
evo::EggCfg brain_cfg(int layers,int D,int sig){evo::EggCfg c;c.L=layers;c.D=D;c.sig=sig;c.V=w::World::kV;c.Din=w::World::kDin;return c;}
void copy_weights(evo::Egg& d,const evo::Egg& s){
  for(std::size_t k=0;k<d.mats.size();++k){d.mats[k]->w=s.mats[k]->w;}
  for(std::size_t k=0;k<d.vecs.size();++k){d.vecs[k]->w=s.vecs[k]->w;}
  d.repack();
}
struct Lin{std::unique_ptr<evo::Egg> g;evo::Noise nz;bool active=false;std::uint64_t id=0;int age=0;};
}  // namespace

int main(int argc,char** argv){
  int epochs=100,P=128,H=200,threads=4,layers=2,D=64,sig=4,archive_every=50,seeds=5,ticks=2000,Kmax=1,split_at=-1,hist_frac=0,hist_founders=8;double alpha_floor=0;
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
    else if(!std::strcmp(s,"-lineages"))Kmax=std::atoi(v);else if(!std::strcmp(s,"-split_at"))split_at=std::atoi(v);else if(!std::strcmp(s,"-nact"))wp.nact=std::atoi(v);
    else if(!std::strcmp(s,"-hist_frac"))hist_frac=std::atoi(v);else if(!std::strcmp(s,"-hist_founders"))hist_founders=std::atoi(v);
    else if(!std::strcmp(s,"-tourney")&&a+2<argc){ta=argv[a+1];tb=argv[a+2];++a;}
    else{std::fprintf(stderr,"unknown option %s\n",s);return 2;}}
  if(wp.S%16){std::fprintf(stderr,"-S must be a multiple of 16\n");return 2;}
  if(split_at<0)split_at=wp.S*3/4;
  const evo::EggCfg bc=brain_cfg(layers,D,sig);

  if(!ta.empty()){  // tournament: A = lineage 0, B = lineage 1
    evo::Egg A(bc,1),B(bc,2);A.load(ta);B.load(tb);double sum=0;
    for(int k=0;k<seeds;++k){w::WParams q=wp;q.K=2;q.seed=seed0+k;w::World W;W.init(q,founders);
      evo::Rows rows;rows.init(A,2*q.S);evo::Stepper sa(A,nullptr,rows,0,q.S),sb(B,nullptr,rows,q.S,q.S);
      std::vector<w::World> one{W};std::vector<evo::Stepper*> st{&sa,&sb};run_worlds(one,rows,st,q.S,ticks,nullptr);W=one[0];
      const long ba=W.biomass(0),bb=W.biomass(1);const double sh=ba+bb>0?(double)ba/(ba+bb):0.5;sum+=sh;
      std::printf("seed %llu\tshareA %.4f\tbiomass %ld %ld\tcount %d %d\n",(unsigned long long)q.seed,sh,ba,bb,W.count(0),W.count(1));}
    std::printf("mean_shareA %.4f\n",sum/seeds);return 0;
  }

  const bool hist=hist_frac>0;const int K=Kmax+(hist?1:0),Kh=Kmax;  // lineage slot Kh holds the injected ancestor
  wp.K=K;wp.seed=seed;w::World canon;canon.init(wp,0);canon.reseed_lineage(0,founders,0);
  std::vector<Lin> lin(Kmax);lin[0].g=std::make_unique<evo::Egg>(bc,seed);lin[0].active=true;std::uint64_t next_lid=1;
  evo::Rows rc;rc.init(*lin[0].g,K*wp.S);
  std::vector<std::unique_ptr<evo::Egg>> anc;  // archive of champion means for historical self-play
  std::FILE* lg=logf.empty()?stdout:std::fopen(logf.c_str(),"w");if(!lg)return 2;
  if(Kmax==1&&!hist)std::fprintf(lg,"# W0 L%d S%d founders %d H%d pairs %d brain L%d D%d sig%d params %ld threads %d seed %llu regrow %d base %d patches %d\n",
    wp.L,wp.S,founders,H,P,layers,D,sig,lin[0].g->nparams(),threads,(unsigned long long)seed,wp.regrow,wp.base,wp.patches);
  else std::fprintf(lg,"# W1 L%d S%d founders %d H%d pairs %d brain L%d D%d sig%d params %ld threads %d seed %llu regrow %d base %d patches %d lineages %d nact %d hist_frac %d hist_founders %d split_at %d\n",
    wp.L,wp.S,founders,H,P,layers,D,sig,lin[0].g->nparams(),threads,(unsigned long long)seed,wp.regrow,wp.base,wp.patches,Kmax,wp.nact,hist_frac,hist_founders,split_at);
  if(!archive.empty())lin[0].g->save(archive+"/egg_0.bin",0);
  int reseeds=0,splits=0,extinct=0;const int Ls=bc.L*bc.D;
  for(int e=0;e<epochs;++e){
    const auto t0=std::chrono::steady_clock::now();
    for(int k=0;k<Kmax;++k)if(lin[k].active)lin[k].nz.gen(*lin[k].g,P,lin[k].id==0?seed:mixu(seed*31+lin[k].id),e);
    const int F2=2*P;std::vector<std::vector<long>> fit(Kmax,std::vector<long>(F2,0));
    const evo::Egg* an=(hist&&!anc.empty())?anc[mixu(seed^((std::uint64_t)e<<8))%anc.size()].get():nullptr;
    const int T=std::max(1,std::min(threads,F2));std::vector<std::thread> th;
    for(int ti=0;ti<T;++ti){const int f0=F2*ti/T,f1=F2*(ti+1)/T;
      th.emplace_back([&,f0,f1]{const int nw=f1-f0;std::vector<w::World> ws(nw,canon);evo::Rows r;r.init(*lin[0].g,K*nw*wp.S);
        for(int i=0;i<nw;++i){const int f=f0+i,p=f/2;
          if(an&&p%hist_frac==0)ws[i].reseed_lineage(Kh,hist_founders,(std::uint64_t)e*P+p);  // same placement in both forks of the pair
          for(int k=0;k<K;++k)for(int s=0;s<wp.S;++s){const int row=(k*nw+i)*wp.S+s;r.pair[row]=p;r.sign[row]=k<Kmax?((f%2==0)?1:-1):0;
            std::copy(rc.state.begin()+(std::size_t)(k*wp.S+s)*Ls,rc.state.begin()+(std::size_t)(k*wp.S+s+1)*Ls,r.state.begin()+(std::size_t)row*Ls);}}
        std::vector<std::unique_ptr<evo::Stepper>> own;std::vector<evo::Stepper*> st(K,nullptr);
        for(int k=0;k<Kmax;++k)if(lin[k].active){own.push_back(std::make_unique<evo::Stepper>(*lin[k].g,&lin[k].nz,r,k*nw*wp.S,nw*wp.S));st[k]=own.back().get();}
        if(an){own.push_back(std::make_unique<evo::Stepper>(*an,nullptr,r,Kh*nw*wp.S,nw*wp.S));st[Kh]=own.back().get();}
        std::vector<std::vector<long>> integ(K,std::vector<long>(nw,0));run_worlds(ws,r,st,wp.S,H,&integ);
        for(int k=0;k<Kmax;++k)for(int i=0;i<nw;++i)fit[k][f0+i]=integ[k][i]/H;});}
    for(auto& x:th)x.join();
    long ch=0;double alpha=0,m=0,m2=0;int nact_l=0;
    for(int k=0;k<Kmax;++k){if(!lin[k].active)continue;std::vector<int> F(P);
      for(int p=0;p<P;++p){F[p]=fit[k][2*p]>fit[k][2*p+1]?1:fit[k][2*p]<fit[k][2*p+1]?-1:0;}
      if(k==0)for(long f:fit[k]){m+=f;m2+=(double)f*f;}
      const double al=std::max(alpha_floor,1.0/(0.015*lin[k].age+1));if(k==0)alpha=al;ch+=evo::egg_update(*lin[k].g,lin[k].nz,F,al);++lin[k].age;++nact_l;}
    m/=F2;const double sd=std::sqrt(std::max(0.0,m2/F2-m*m));
    {std::vector<w::World> one{canon};std::vector<std::unique_ptr<evo::Stepper>> own;std::vector<evo::Stepper*> st(K,nullptr);
      for(int k=0;k<Kmax;++k)if(lin[k].active){own.push_back(std::make_unique<evo::Stepper>(*lin[k].g,nullptr,rc,k*wp.S,wp.S));st[k]=own.back().get();}
      run_worlds(one,rc,st,wp.S,H,nullptr);canon=one[0];}
    // lineage dynamics: extinction (or reseed if nothing is left), then splits
    int alive_l=0;for(int k=0;k<Kmax;++k)if(lin[k].active&&canon.count(k)>0)++alive_l;
    for(int k=0;k<Kmax;++k){if(!lin[k].active||canon.count(k)>0)continue;
      if(alive_l==0&&k==0){canon.reseed_lineage(0,founders,(std::uint64_t)e+1);std::fill(rc.state.begin(),rc.state.end(),0);++reseeds;continue;}
      if(!archive.empty())lin[k].g->save(archive+"/extinct_L"+std::to_string(lin[k].id)+"_e"+std::to_string(e+1)+".bin",e+1);
      lin[k].active=false;lin[k].g.reset();++extinct;}
    for(int k=0;k<Kmax;++k){if(!lin[k].active||canon.count(k)<split_at)continue;int j=-1;for(int q=0;q<Kmax;++q)if(!lin[q].active){j=q;break;}if(j<0)break;
      lin[j].g=std::make_unique<evo::Egg>(bc,seed+next_lid);copy_weights(*lin[j].g,*lin[k].g);
      std::uint64_t c=0;for(auto* M:lin[j].g->mats)for(auto& x:M->w){const std::uint64_t h=mixu(mixu(seed^0x5b1ULL)+(next_lid<<40)+c++);if(h%10==0)x=(std::int8_t)std::clamp((int)x+((h>>8)&1?1:-1),-127,127);}
      lin[j].g->repack();
      lin[j].active=true;lin[j].id=next_lid++;lin[j].age=0;++splits;int moved=0,t=0;
      for(int s=0;s<wp.S;++s){if(!canon.ag[(std::size_t)k*wp.S+s].alive)continue;if((moved++)%2==0)continue;
        while(t<wp.S&&canon.ag[(std::size_t)j*wp.S+t].alive){++t;}
        if(t>=wp.S)break;
        canon.move_slot(k*wp.S+s,j*wp.S+t);
        std::copy(rc.state.begin()+(std::size_t)(k*wp.S+s)*Ls,rc.state.begin()+(std::size_t)(k*wp.S+s+1)*Ls,rc.state.begin()+(std::size_t)(j*wp.S+t)*Ls);}}
    // champion (most abundant lineage): archived for tournaments and historical self-play
    int ck=-1;for(int k=0;k<Kmax;++k)if(lin[k].active&&(ck<0||canon.count(k)>canon.count(ck)))ck=k;
    const double sec=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
    long cb=0;int cp=0;for(int k=0;k<Kmax;++k){cb+=canon.biomass(k);cp+=canon.count(k);}
    if(Kmax==1)std::fprintf(lg,"%d\t%d\t%ld\t%.1f\t%.1f\t%ld\t%.4f\t%.2f\t%d\t%ld\n",e+1,cp,cb,m,sd,ch,alpha,sec,reseeds,canon.resources());
    else{std::string cs;for(int k=0;k<Kmax;++k)if(lin[k].active)cs+=(cs.empty()?"":",")+std::to_string(lin[k].id)+":"+std::to_string(canon.count(k));
      std::fprintf(lg,"%d\t%d\t%ld\t%.1f\t%.1f\t%ld\t%.4f\t%.2f\t%d\t%ld\t%d\t%d\t%d\t%s\n",e+1,cp,cb,m,sd,ch,alpha,sec,reseeds,canon.resources(),nact_l,splits,extinct,cs.c_str());}
    std::fflush(lg);
    if(((e+1)%archive_every==0||e+1==epochs)&&ck>=0){
      if(!archive.empty())lin[ck].g->save(archive+"/egg_"+std::to_string(e+1)+".bin",e+1);
      if(hist){auto cpy=std::make_unique<evo::Egg>(bc,1);copy_weights(*cpy,*lin[ck].g);anc.push_back(std::move(cpy));if(anc.size()>32)anc.erase(anc.begin());}}
  }
  if(lg!=stdout)std::fclose(lg);
}
