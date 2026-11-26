// World v1 (docs/WORLD.md): lineages evolved by integer EGGROLL in mirrored forks of a persistent world;
// plus ancestor tournaments.
// Train:   oee_w -epochs N [-pairs 128] [-H 200] [-L 32] [-S 64] [-founders 16] [-layers 2] [-D 64] [-sig 4] [-seed 1]
//                [-regrow 4] [-base 1] [-patches 6] [-threads 4] [-alpha_floor 0] [-archive DIR] [-archive_every 50]
//                [-lineages 1] [-split_at 3S/4] [-nact 6] [-hist_frac 0] [-hist_founders 8] [-log FILE]
//                [-ck FILE] [-ck_every 10] [W2a: -season T -view R -signals 1 -rot 1 -sig_cost 0 -climate_every 5]
//                [-split_age 1: a split lineage takes its parent's age, so its update fraction alpha (W1b)]
//                [-null_es 1: updates use random pair signs instead of fitness signs (null control, W1c)]
//                [-hist_pair 1: every fork pair holds an ancestor drawn for that pair from the archive (W1e)]
//                [W3: -matter 1 -nact 10 [-m_steps 8 -m_decay 200 -mk_cost 10 -wr_cost 1 -feed_amt 20 -rep_cost 16 -wall_cost 32]]
//                [W3.1: -spawn R (random programs per 1000 ticks per 1024 cells) -mut M (copy error per byte, /65536)]
//                [W3.2: -own 1 (machines belong to the maker's lineage; private granaries)]
//          W0 = -lineages 1 -nact 6 -hist_frac 0 (bit-identical to the W0 binary).
//          W1 = -lineages K -nact 7 (attack) -hist_frac 4 (ancestor in every 4th fork pair).
//          -ck: checkpoint every ck_every epochs and at the end. If FILE exists at start, the run resumes from it
//          (bit-exact with any thread count; the log is cut back to the checkpoint epoch and appended).
//          Log: epoch canon_pop canon_biomass fork_fit_mean fork_fit_sd changed alpha sec reseeds canon_resources
//               [W1: active splits extinct lineage_id:count,...]. Fork fitness = biomass of all evolving lineages
//               summed over the episode / H; alpha of the first active lineage; active = lineages alive after the epoch.
// Tourney: oee_w -tourney A.egg B.egg [-seeds 5] [-ticks 2000] [-founders 16] [-L 32] [-S 64] [-seed0 1000] [-nact 6]
//                [-threads 4] [-ablate amnesia] (world flags must match training)
//                [-climate_ticks 1000] [-stats 1]
//          -stats 1: per-genome behaviour over all games (action fractions, kills, births and deaths per agent-tick).
//          -curve N: also print A's mean biomass share at every N ticks (over all games).
//          Generality (W3; docs/WORLD.md §6): -gen 1 draws each arena's world settings at random (side 24-64, 3-10
//          patches, season period 150-600 if seasons are on, machine costs x0.5-2); -arena_progs N seeds N random
//          programs (short ones likelier, -prog_cont 0.7); -solo 1 plays A and B alone in the same arenas and reports
//          the mean log ratio of their post-founder biomass (an absolute measure, as in Legg & Hutter's universal
//          intelligence, arXiv:0712.3329, rather than a head-to-head share).
//          -swap_at T (with -solo 1 and -arena_progs): at tick T the arena's own programs are replaced by new random
//          ones (an intervention on one mechanism of the world). Recovery = biomass integrated over the -swap_win
//          ticks after T / over the swap_win ticks before; reported as the mean log ratio of A's recovery to B's
//          (faster adaptation to a changed mechanism suggests a causal model; Bengio et al., arXiv:1901.10912).
//          mean_lshareA: integrated share over ticks after the founder generation (> skip, default max_age = 500); the
//          founders' starting energy dominates the integrated share before that (docs/WORLD.md §9).
//          -ablate amnesia: A's recurrent state is zeroed before every step (A without memory). -ablate shuffle: A's
//          agents swap states every step (state values kept, own history lost: the stricter memory test). -ablate deaf: A hears
//          no symbols. -ablate wipe (W3): every 100 ticks, all machines made by A or copied from them are removed.
//          With A = B this measures whether the genome uses the mechanism: ishareA < 0.5 means it pays.
//          With seasons, each arena draws a new climate every climate_ticks ticks (training: climate_every*H).
//          Each seed is played twice with the lineage slots swapped (mirrored). Per game: final biomass share of A and
//          integrated share (A's biomass summed over all ticks / both summed). Summary: mean_shareA (final share over
//          games); mean_ishareA over seeds (mean of each mirrored pair) with SE, t against 0.5 and wins.
// Epoch: copy the canonical world into 2P forks; fork 2p uses +noise, 2p+1 the same noise negated (same world seed:
// common random numbers); each lineage has its own noise; fitness of lineage k = its biomass summed over the H ticks;
// F_kp = sign(fit_k,2p - fit_k,2p+1); integer EGGROLL update per lineage; the canonical world then advances H ticks.
// Brains are stepped only for living agents (dead slots are skipped; same results as stepping all slots).
// Lineage dynamics (W1): a lineage with no agents left goes extinct (archived); if no lineage has agents left, the
// first active one is reseeded instead. A lineage at >= split_at agents splits (copy of its mean with 10% of weights
// moved one step; half of its agents move to the new lineage).
// Historical self-play (W1): every hist_frac-th fork pair also contains hist_founders agents of one archived ancestor
// mean (no noise, not updated), as in Digital Red Queen (arXiv:2601.03335). Ancestors are added every archive_every
// epochs (last 32 kept).
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "evo/egg.hpp"
#include "w/world.hpp"

using namespace oee;

namespace {
std::uint64_t mixu(std::uint64_t x){x+=0x9e3779b97f4a7c15ULL;x=(x^(x>>30))*0xbf58476d1ce4e5b9ULL;x=(x^(x>>27))*0x94d049bb133111ebULL;return x^(x>>31);}
// Run H ticks. Rows layout: row(k,i,s) = (k*nw + i)*S + s; st[k] null for lineages without agents in these worlds.
// Only living agents are observed and stepped. integ[k][i] (optional): lineage-k biomass summed over the ticks in world i.
// amn (optional, analysis), per lineage: 1 = recurrent state zeroed before every step (no memory); 2 = states rotated
// among the lineage's living agents before every step (realistic state values, but not the agent's own history).
// pf (optional): lineage kp is stepped by one stepper per world (pf[i] covers rows (kp*nw+i)*S.. of world i), e.g. a
// different ancestor in each fork pair; st[kp] must then be non-null (any of them).
void run_worlds(std::vector<w::World>& ws,evo::Rows& rows,const std::vector<evo::Stepper*>& st,int S,int H,std::vector<std::vector<long>>* integ,
                const std::vector<char>* amn=nullptr,const std::vector<evo::Stepper*>* pf=nullptr,int kp=-1){
  const int nw=(int)ws.size(),K=(int)st.size(),Din=ws[0].din(),V=ws[0].nv();const int L=(int)(rows.state.size()/rows.R);
  std::vector<std::int8_t> obs((std::size_t)nw*S*Din),lgt((std::size_t)K*nw*S*V,0);std::vector<int> idx,born;
  const std::vector<std::int8_t> zero((std::size_t)S*V,0);std::vector<const std::int8_t*> lp(K);
  for(int t=0;t<H;++t){
    for(int k=0;k<K;++k){if(!st[k])continue;idx.clear();
      if(pf&&k==kp){for(int i=0;i<nw;++i){if(!(*pf)[i])continue;idx.clear();  // one stepper per world
          for(int s=0;s<S;++s)if(ws[i].ag[(std::size_t)k*S+s].alive){ws[i].observe(k*S+s,obs.data()+idx.size()*Din);idx.push_back(s);}
          if(idx.empty())continue;const std::int8_t* o=(*pf)[i]->step(obs.data(),idx.data(),(int)idx.size());
          for(std::size_t j=0;j<idx.size();++j)std::memcpy(lgt.data()+(((std::size_t)k*nw+i)*S+idx[j])*V,o+j*V,V);}
        continue;}
      for(int i=0;i<nw;++i)for(int s=0;s<S;++s)if(ws[i].ag[(std::size_t)k*S+s].alive){ws[i].observe(k*S+s,obs.data()+idx.size()*Din);idx.push_back(i*S+s);}
      if(idx.empty())continue;
      if(amn&&(*amn)[k]==1)for(int j:idx)std::fill(rows.state.begin()+((std::size_t)k*nw*S+j)*L,rows.state.begin()+((std::size_t)k*nw*S+j+1)*L,0);
      if(amn&&(*amn)[k]==2&&idx.size()>1){std::vector<std::int8_t> t0(rows.state.begin()+((std::size_t)k*nw*S+idx[0])*L,rows.state.begin()+((std::size_t)k*nw*S+idx[0]+1)*L);
        for(std::size_t j=0;j+1<idx.size();++j)std::copy(rows.state.begin()+((std::size_t)k*nw*S+idx[j+1])*L,rows.state.begin()+((std::size_t)k*nw*S+idx[j+1]+1)*L,rows.state.begin()+((std::size_t)k*nw*S+idx[j])*L);
        std::copy(t0.begin(),t0.end(),rows.state.begin()+((std::size_t)k*nw*S+idx.back())*L);}
      const std::int8_t* o=st[k]->step(obs.data(),idx.data(),(int)idx.size());
      for(std::size_t j=0;j<idx.size();++j)std::memcpy(lgt.data()+((std::size_t)k*nw*S+idx[j])*V,o+j*V,V);}
    for(int i=0;i<nw;++i){for(int k=0;k<K;++k)lp[k]=st[k]?lgt.data()+((std::size_t)k*nw+i)*S*V:zero.data();born.clear();ws[i].tick(lp.data(),born);
      for(int b:born){const int k=b/S,s=b%S;std::fill(rows.state.begin()+((std::size_t)(k*nw+i)*S+s)*L,rows.state.begin()+((std::size_t)(k*nw+i)*S+s+1)*L,0);}
      if(integ)for(int k=0;k<K;++k)if(st[k])(*integ)[k][i]+=ws[i].biomass(k);}
  }
}
evo::EggCfg brain_cfg(int layers,int D,int sig,int din,int V){evo::EggCfg c;c.L=layers;c.D=D;c.sig=sig;c.V=V;c.Din=din;return c;}
void copy_weights(evo::Egg& d,const evo::Egg& s){
  for(std::size_t k=0;k<d.mats.size();++k){d.mats[k]->w=s.mats[k]->w;}
  for(std::size_t k=0;k<d.vecs.size();++k){d.vecs[k]->w=s.vecs[k]->w;}
  d.repack();
}
struct Lin{std::unique_ptr<evo::Egg> g;evo::Noise nz;bool active=false;std::uint64_t id=0;int age=0;};
constexpr std::int64_t kCkMagic=0x3130574b43454f4fLL;  // "OEECKW01"
}  // namespace

int main(int argc,char** argv){
  int epochs=100,P=128,H=200,threads=4,layers=2,D=64,sig=4,archive_every=50,seeds=5,ticks=2000,Kmax=1,split_at=-1,hist_frac=0,hist_founders=8,ck_every=10,climate_every=5,climate_ticks=1000,stats=0,curve=0,skip=-1,split_age=0,null_es=0,hist_pair=0,gen=0,arena_progs=0,solo=0,swap_at=0,swap_win=500;double prog_cont=0.7;
  double alpha_floor=0;std::uint64_t seed=1,seed0=1000;std::string logf,archive,ta,tb,ckf,ablate;w::WParams wp;int founders=16;
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
    else if(!std::strcmp(s,"-ablate"))ablate=v;else if(!std::strcmp(s,"-ck"))ckf=v;
    else if(!std::strcmp(s,"-season"))wp.season=std::atoi(v);else if(!std::strcmp(s,"-view"))wp.view=std::atoi(v);
    else if(!std::strcmp(s,"-signals"))wp.signals=std::atoi(v);else if(!std::strcmp(s,"-rot"))wp.rot=std::atoi(v);
    else if(!std::strcmp(s,"-sig_cost"))wp.sig_cost=std::atoi(v);else if(!std::strcmp(s,"-climate_every"))climate_every=std::max(1,std::atoi(v));
    else if(!std::strcmp(s,"-climate_ticks"))climate_ticks=std::max(1,std::atoi(v));
    else if(!std::strcmp(s,"-matter"))wp.matter=std::atoi(v);else if(!std::strcmp(s,"-m_steps"))wp.m_steps=std::atoi(v);
    else if(!std::strcmp(s,"-m_decay"))wp.m_decay=std::atoi(v);else if(!std::strcmp(s,"-mk_cost"))wp.mk_cost=std::atoi(v);
    else if(!std::strcmp(s,"-wr_cost"))wp.wr_cost=std::atoi(v);else if(!std::strcmp(s,"-feed_amt"))wp.feed_amt=std::atoi(v);
    else if(!std::strcmp(s,"-rep_cost"))wp.rep_cost=std::atoi(v);else if(!std::strcmp(s,"-wall_cost"))wp.wall_cost=std::atoi(v);
    else if(!std::strcmp(s,"-spawn"))wp.spawn=std::atoi(v);else if(!std::strcmp(s,"-mut"))wp.mut=std::atoi(v);else if(!std::strcmp(s,"-own"))wp.own=std::atoi(v);else if(!std::strcmp(s,"-stats"))stats=std::atoi(v);else if(!std::strcmp(s,"-curve"))curve=std::max(0,std::atoi(v));else if(!std::strcmp(s,"-skip"))skip=std::max(0,std::atoi(v));else if(!std::strcmp(s,"-split_age"))split_age=std::atoi(v);else if(!std::strcmp(s,"-null_es"))null_es=std::atoi(v);else if(!std::strcmp(s,"-hist_pair"))hist_pair=std::atoi(v);
    else if(!std::strcmp(s,"-gen"))gen=std::atoi(v);else if(!std::strcmp(s,"-arena_progs"))arena_progs=std::atoi(v);
    else if(!std::strcmp(s,"-prog_cont"))prog_cont=std::atof(v);else if(!std::strcmp(s,"-solo"))solo=std::atoi(v);
    else if(!std::strcmp(s,"-swap_at"))swap_at=std::atoi(v);else if(!std::strcmp(s,"-swap_win"))swap_win=std::max(1,std::atoi(v));else if(!std::strcmp(s,"-ck_every"))ck_every=std::max(1,std::atoi(v));
    else if(!std::strcmp(s,"-tourney")&&a+2<argc){ta=argv[a+1];tb=argv[a+2];++a;}
    else{std::fprintf(stderr,"unknown option %s\n",s);return 2;}}
  if(wp.S%16){std::fprintf(stderr,"-S must be a multiple of 16\n");return 2;}
  if(split_at<0)split_at=wp.S*3/4;
  if(wp.matter&&wp.nact<10){std::fprintf(stderr,"-matter 1 needs -nact 10 (write, make/unmake, feed)\n");return 2;}
  const evo::EggCfg bc=brain_cfg(layers,D,sig,(wp.signals||wp.matter)?128:w::World::kDin,wp.matter?32:w::World::kV);
  const bool w2=wp.season>0||wp.signals>0||wp.view!=2||wp.matter>0;

  if(!ta.empty()){  // tournament, mirrored: game 2k has A in slot 0, game 2k+1 has A in slot 1 (same arena seed)
    if(!ablate.empty()&&ablate!="amnesia"&&ablate!="shuffle"&&ablate!="deaf"&&ablate!="wipe"){std::fprintf(stderr,"unknown ablation %s\n",ablate.c_str());return 2;}
    if(ablate=="wipe"&&!wp.matter){std::fprintf(stderr,"-ablate wipe needs -matter 1\n");return 2;}
    if(ablate=="deaf"&&!wp.signals){std::fprintf(stderr,"-ablate deaf needs -signals 1\n");return 2;}
    evo::Egg A(bc,1),B(bc,2);A.load(ta);B.load(tb);const int G=2*seeds;
    std::vector<double> fs(G),is(G);std::vector<long> ba(G),bb(G);std::vector<int> na(G),nb(G);std::vector<std::vector<long>> gst(G);
    const int nc=curve>0?(ticks+curve-1)/curve:0;std::vector<std::vector<double>> cv(G,std::vector<double>(nc,0.5));
    const int sk=skip>=0?skip:wp.max_age;std::vector<double> ls(G,0.5);
    if(arena_progs>0&&!wp.matter){std::fprintf(stderr,"-arena_progs needs -matter 1\n");return 2;}
    if(swap_at>0&&(!solo||arena_progs<=0||swap_at<swap_win||swap_at+swap_win>ticks)){std::fprintf(stderr,"-swap_at needs -solo 1, -arena_progs, and swap_win <= swap_at <= ticks - swap_win\n");return 2;}
    std::vector<double> sv(G,0),rv(G,0);  // solo: post-founder integrated biomass alone; swap: log recovery
    auto game=[&](int gi){const int sw=gi%2;w::WParams q=wp;q.K=solo?1:2;q.seed=seed0+gi/2;
      if(gen){const std::uint64_t h=mixu(mixu(q.seed^0x6e9ULL));q.L=24+8*(int)(h%6);q.patches=3+(int)((h>>8)%8);if(wp.season>0)q.season=150+(int)((h>>16)%451);
        const double f=std::exp2(((int)((h>>32)%201)-100)/100.0);  // machine costs x0.5-2
        q.mk_cost=std::max(1,(int)std::lround(wp.mk_cost*f));q.rep_cost=std::max(1,(int)std::lround(wp.rep_cost*f));q.wall_cost=std::max(1,(int)std::lround(wp.wall_cost*f));}
      w::World W;W.init(q,founders);if(arena_progs>0)W.seed_programs((int)std::lround(arena_progs*(double)q.L*q.L/1024),q.seed,100,prog_cont);  // density as at side 32
      evo::Rows rows;rows.init(A,q.K*q.S);evo::Stepper s0(sw?B:A,nullptr,rows,0,q.S);std::unique_ptr<evo::Stepper> s1;
      if(!solo)s1=std::make_unique<evo::Stepper>(sw?A:B,nullptr,rows,q.S,q.S);
      std::vector<w::World> one{W};std::vector<evo::Stepper*> st{&s0};if(!solo)st.push_back(s1.get());std::vector<std::vector<long>> ig(q.K,std::vector<long>(1,0));
      const int sa=solo?0:sw;  // A's lineage slot (solo: the only slot, A in even games)
      std::vector<char> amn(q.K,0);if(!solo||sw==0){amn[sa]=ablate=="amnesia"?1:ablate=="shuffle"?2:0;if(ablate=="deaf")one[0].deaf=1u<<sa;}one[0].track=stats>0;
      int step=curve>0?std::min(curve,climate_ticks):climate_ticks;long la=-1,lb=-1;if(ablate=="wipe")step=std::min(step,100);  // wipe A's machines every 100 ticks
      long sp0=-1,sp1=-1,sp2=-1;  // swap test: integrated biomass at swap_at - swap_win, swap_at, swap_at + swap_win
      for(int t0=0;t0<ticks;){int n=std::min(step-t0%step,ticks-t0);if(t0<sk)n=std::min(n,sk-t0);  // segment ends at curve points, climate changes and skip
        if(swap_at>0)for(int b:{swap_at-swap_win,swap_at,swap_at+swap_win})if(t0<b)n=std::min(n,b-t0);
        if(t0>0&&t0%climate_ticks==0)one[0].set_climate((std::uint64_t)(t0/climate_ticks));
        run_worlds(one,rows,st,q.S,n,&ig,&amn);t0+=n;if(t0==sk){la=ig[sa][0];lb=solo?0:ig[1-sw][0];}
        if(ablate=="wipe"&&t0%100==0&&(!solo||sw==0))one[0].wipe(sa);
        if(swap_at>0){if(t0==swap_at-swap_win)sp0=ig[0][0];if(t0==swap_at){sp1=ig[0][0];one[0].swap_programs(q.seed^0x5a5aULL,100,prog_cont);}if(t0==swap_at+swap_win)sp2=ig[0][0];}
        if(curve>0&&t0%curve==0&&!solo){const long x=one[0].biomass(sw),y=one[0].biomass(1-sw);cv[gi][t0/curve-1]=x+y>0?(double)x/(x+y):0.5;}}
      if(solo){sv[gi]=(double)(ig[0][0]-std::max(0L,la));ba[gi]=one[0].biomass(0);na[gi]=one[0].count(0);
        if(swap_at>0)rv[gi]=std::log((double)(sp2-sp1+1)/(double)(sp1-sp0+1));return;}
      if(la>=0){const long x=ig[sw][0]-la,y=ig[1-sw][0]-lb;ls[gi]=x+y>0?(double)x/(x+y):0.5;}

      const w::World& Z=one[0];const int a=sw,b=1-sw;  // lineage slots of A and B
      ba[gi]=Z.biomass(a);bb[gi]=Z.biomass(b);na[gi]=Z.count(a);nb[gi]=Z.count(b);
      if(stats){gst[gi].assign(64,0);for(int j=0;j<32;++j){gst[gi][j]=Z.stat[(std::size_t)a*32+j];gst[gi][32+j]=Z.stat[(std::size_t)b*32+j];}}
      fs[gi]=ba[gi]+bb[gi]>0?(double)ba[gi]/(ba[gi]+bb[gi]):0.5;is[gi]=ig[a][0]+ig[b][0]>0?(double)ig[a][0]/(ig[a][0]+ig[b][0]):0.5;};
    const int T=std::max(1,std::min(threads,G));std::vector<std::thread> th;
    for(int ti=0;ti<T;++ti)th.emplace_back([&,ti]{for(int gi=ti;gi<G;gi+=T)game(gi);});
    for(auto& x:th)x.join();
    if(solo){double s1=0,s2=0,ma=0,mb=0;int w=0;
      for(int k=0;k<seeds;++k){const double a=sv[2*k],b=sv[2*k+1];ma+=a;mb+=b;const double d=std::log((a+1)/(b+1));s1+=d;s2+=d*d;w+=d>0;
        std::printf("seed %llu\tsoloA %.0f\tsoloB %.0f\tlog_ratio %.4f\tcountA %d\tcountB %d\n",(unsigned long long)(seed0+k),a,b,d,na[2*k],na[2*k+1]);}
      const double m=s1/seeds,se=seeds>1?std::sqrt(std::max(0.0,(s2-seeds*m*m)/(seeds-1))/seeds):0;
      std::vector<double> dl;int ua=0,ub=0;for(int k=0;k<seeds;++k){dl.push_back(std::log((sv[2*k]+1)/(sv[2*k+1]+1)));ua+=na[2*k]>0;ub+=na[2*k+1]>0;}
      std::sort(dl.begin(),dl.end());const double med=seeds%2?dl[seeds/2]:(dl[seeds/2-1]+dl[seeds/2])/2;
      std::printf("solo_log_ratio %.4f\tse %.4f\tt %.2f\twins %d/%d\tmedian %.4f\tsurviveA %d/%d\tsurviveB %d/%d\tmeanA %.0f\tmeanB %.0f\tafter tick %d\n",
        m,se,se>1e-9?m/se:0.0,w,seeds,med,ua,seeds,ub,seeds,ma/seeds,mb/seeds,sk);
      if(swap_at>0){double r1=0,r2=0,ra=0,rb=0;int rw=0;  // recovery after the intervention: A against B in the same arenas
        for(int k=0;k<seeds;++k){const double d=rv[2*k]-rv[2*k+1];r1+=d;r2+=d*d;rw+=d>0;ra+=rv[2*k];rb+=rv[2*k+1];}
        const double mr=r1/seeds,sr=seeds>1?std::sqrt(std::max(0.0,(r2-seeds*mr*mr)/(seeds-1))/seeds):0;
        std::printf("swap_recovery %.4f\tse %.4f\tt %.2f\twins %d/%d\tlogrecA %.4f\tlogrecB %.4f\tat tick %d window %d\n",mr,sr,sr>1e-9?mr/sr:0.0,rw,seeds,ra/seeds,rb/seeds,swap_at,swap_win);}
      return 0;}
    double sf=0,si=0,si2=0;int wins=0;
    for(int gi=0;gi<G;++gi){sf+=fs[gi];std::printf("seed %llu\tswap %d\tshareA %.4f\tishareA %.4f\tbiomass %ld %ld\tcount %d %d\n",
      (unsigned long long)(seed0+gi/2),gi%2,fs[gi],is[gi],ba[gi],bb[gi],na[gi],nb[gi]);}
    for(int k=0;k<seeds;++k){const double d=(is[2*k]+is[2*k+1])/2;si+=d;si2+=d*d;wins+=d>0.5+1e-9;}
    const double mi=si/seeds,se=seeds>1?std::sqrt(std::max(0.0,(si2-seeds*mi*mi)/(seeds-1))/seeds):0;
    if(stats){static const char* nm[]={"noop","fwd","left","right","eat","repro","attack","write","make","feed"};
      for(int g=0;g<2;++g){long tot[32]={0};for(int gi=0;gi<G;++gi)for(int j=0;j<32;++j)tot[j]+=gst[gi][32*g+j];const double T=std::max(1L,tot[22]);
        std::printf("stats %s agent_ticks %ld",g?"B":"A",tot[22]);for(int j=0;j<wp.nact&&j<10;++j)std::printf(" %s %.3f",nm[j],tot[j]/T);
        std::printf(" kills/kt %.2f births/kt %.2f starved/kt %.2f aged/kt %.2f killed/kt %.2f symbol %.3f\n",1e3*tot[16]/T,1e3*tot[17]/T,1e3*tot[18]/T,1e3*tot[19]/T,1e3*tot[20]/T,tot[21]/T);}}
    if(curve>0){std::printf("curve");for(int c=0;c<nc;++c){double m=0;for(int gi=0;gi<G;++gi)m+=cv[gi][c];std::printf(" %d:%.3f",(c+1)*curve,m/G);}std::printf("\n");}
    std::printf("mean_shareA %.4f\n",sf/G);
    std::printf("mean_ishareA %.4f\tse %.4f\tt %.2f\twins %d/%d\n",mi,se,se>1e-9?(mi-0.5)/se:0.0,wins,seeds);
    {double s1=0,s2=0;int w=0;for(int k=0;k<seeds;++k){const double d=(ls[2*k]+ls[2*k+1])/2;s1+=d;s2+=d*d;w+=d>0.5+1e-9;}
      const double ml=s1/seeds,sl=seeds>1?std::sqrt(std::max(0.0,(s2-seeds*ml*ml)/(seeds-1))/seeds):0;
      std::printf("mean_lshareA %.4f\tse %.4f\tt %.2f\twins %d/%d\tafter tick %d\n",ml,sl,sl>1e-9?(ml-0.5)/sl:0.0,w,seeds,sk);}
    return 0;
  }

  const bool hist=hist_frac>0;const int K=Kmax+(hist?1:0),Kh=Kmax;  // lineage slot Kh holds the injected ancestor
  wp.K=K;wp.seed=seed;w::World canon;canon.init(wp,0);canon.reseed_lineage(0,founders,0);
  std::vector<Lin> lin(Kmax);lin[0].g=std::make_unique<evo::Egg>(bc,seed);lin[0].active=true;std::uint64_t next_lid=1;
  evo::Rows rc;rc.init(*lin[0].g,K*wp.S);
  std::vector<std::unique_ptr<evo::Egg>> anc;  // archive of champion means for historical self-play
  int reseeds=0,splits=0,extinct=0,e0=0;const int Ls=bc.L*bc.D;
  const std::vector<std::int64_t> fp={Kmax,P,H,founders,layers,D,sig,split_at,hist_frac,hist_founders,archive_every,(std::int64_t)std::llround(alpha_floor*1e9),
    (std::int64_t)seed,wp.L,wp.S,wp.patches,wp.regrow,wp.base,wp.nact};  // settings a resumed run must share
  std::vector<std::int64_t> fp2=fp;if(w2){for(std::int64_t x:{wp.season,wp.view,wp.signals,wp.rot,wp.sig_cost,climate_every})fp2.push_back(x);}
  if(split_age)fp2.push_back(1000+split_age);
  if(null_es)fp2.push_back(2000+null_es);
  if(hist_pair)fp2.push_back(3000+hist_pair);
  if(wp.matter){for(std::int64_t x:{wp.matter,wp.m_steps,wp.m_decay,wp.mk_cost,wp.wr_cost,wp.feed_amt,wp.rep_cost,wp.wall_cost})fp2.push_back(4000+x);}
  if(wp.spawn||wp.mut){fp2.push_back(5000+wp.spawn);fp2.push_back(6000000+wp.mut);}
  if(wp.own)fp2.push_back(7000+wp.own);
  if(hist_pair&&!hist){std::fprintf(stderr,"-hist_pair needs -hist_frac > 0\n");return 2;}
  auto save_ck=[&](int e){const std::string tmp=ckf+".tmp";std::FILE* f=std::fopen(tmp.c_str(),"wb");if(!f)return false;
    auto w=[&](std::int64_t x){std::fwrite(&x,8,1,f);};w(kCkMagic);w((std::int64_t)fp2.size());for(auto x:fp2)w(x);
    w(e);w(reseeds);w(splits);w(extinct);w((std::int64_t)next_lid);canon.save(f);w((std::int64_t)rc.state.size());std::fwrite(rc.state.data(),1,rc.state.size(),f);
    for(auto& l:lin){w(l.active);w((std::int64_t)l.id);w(l.age);if(l.active)l.g->save(f,e);}
    w((std::int64_t)anc.size());for(auto& a:anc)a->save(f,0);
    const bool ok=std::fflush(f)==0&&!std::ferror(f);std::fclose(f);return ok&&std::rename(tmp.c_str(),ckf.c_str())==0;};
  auto load_ck=[&](std::FILE* f)->int{  // epochs done, or -1
    auto r=[&](std::int64_t& x){return std::fread(&x,8,1,f)==1;};std::int64_t x=0,n=0;
    if(!r(x)||x!=kCkMagic||!r(n)||n!=(std::int64_t)fp2.size()){std::fprintf(stderr,"checkpoint has other settings\n");return -1;}
    for(std::size_t i=0;i<fp2.size();++i){if(!r(x))return -1;if(x!=fp2[i]){std::fprintf(stderr,"checkpoint has other settings (field %zu: %lld, now %lld)\n",i,(long long)x,(long long)fp2[i]);return -1;}}
    std::int64_t e,a,b,c,d;if(!r(e)||!r(a)||!r(b)||!r(c)||!r(d))return -1;reseeds=(int)a;splits=(int)b;extinct=(int)c;next_lid=(std::uint64_t)d;
    if(!canon.load(f))return -1;
    if(!r(n)||n!=(std::int64_t)rc.state.size()||std::fread(rc.state.data(),1,rc.state.size(),f)!=rc.state.size())return -1;
    for(auto& l:lin){std::int64_t ac,id,ag;if(!r(ac)||!r(id)||!r(ag))return -1;l.active=ac!=0;l.id=(std::uint64_t)id;l.age=(int)ag;l.g.reset();
      if(l.active){l.g=std::make_unique<evo::Egg>(bc,1);l.g->load(f);}}
    if(!r(n)||n<0||n>1024){return -1;}anc.clear();for(std::int64_t i=0;i<n;++i){anc.push_back(std::make_unique<evo::Egg>(bc,1));anc.back()->load(f);}
    return (int)e;};
  if(!ckf.empty())if(std::FILE* f=std::fopen(ckf.c_str(),"rb")){int e=-1;try{e=load_ck(f);}catch(const std::exception&){e=-1;}std::fclose(f);
    if(e<0){std::fprintf(stderr,"cannot resume from %s\n",ckf.c_str());return 2;}e0=e;}
  if(e0>0&&!logf.empty()){std::vector<std::string> keep;  // cut the log back to the checkpoint
    if(std::FILE* f=std::fopen(logf.c_str(),"r")){static char buf[1<<16];while(std::fgets(buf,sizeof buf,f))if(buf[0]=='#'||std::atoi(buf)<=e0)keep.push_back(buf);std::fclose(f);}
    if(std::FILE* f=std::fopen(logf.c_str(),"w")){for(auto& s:keep)std::fputs(s.c_str(),f);std::fclose(f);}}
  std::FILE* lg=logf.empty()?stdout:std::fopen(logf.c_str(),e0>0?"a":"w");if(!lg)return 2;
  if(e0==0){
    if(Kmax==1&&!hist)std::fprintf(lg,"# W0 L%d S%d founders %d H%d pairs %d brain L%d D%d sig%d params %ld threads %d seed %llu regrow %d base %d patches %d\n",
      wp.L,wp.S,founders,H,P,layers,D,sig,lin[0].g->nparams(),threads,(unsigned long long)seed,wp.regrow,wp.base,wp.patches);
    else std::fprintf(lg,"# W1 L%d S%d founders %d H%d pairs %d brain L%d D%d sig%d params %ld threads %d seed %llu regrow %d base %d patches %d lineages %d nact %d hist_frac %d hist_founders %d split_at %d\n",
      wp.L,wp.S,founders,H,P,layers,D,sig,lin[0].g->nparams(),threads,(unsigned long long)seed,wp.regrow,wp.base,wp.patches,Kmax,wp.nact,hist_frac,hist_founders,split_at);
    if(w2)std::fprintf(lg,"# W2a season %d view %d signals %d rot %d sig_cost %d climate_every %d din %d\n",wp.season,wp.view,wp.signals,wp.rot,wp.sig_cost,climate_every,bc.Din);
    if(wp.matter)std::fprintf(lg,"# W3 matter %d m_steps %d m_decay %d mk_cost %d wr_cost %d feed_amt %d rep_cost %d wall_cost %d spawn %d mut %d V %d; extra columns: machines walls tapes classes\n",
      wp.matter,wp.m_steps,wp.m_decay,wp.mk_cost,wp.wr_cost,wp.feed_amt,wp.rep_cost,wp.wall_cost,wp.spawn,wp.mut,bc.V);
    if(wp.own)std::fprintf(lg,"# W3.2 own %d (machines owned by the maker's lineage; owners eat from their stores)\n",wp.own);
    if(split_age)std::fprintf(lg,"# W1b split_age %d (split lineages take the parent's age)\n",split_age);
    if(null_es)std::fprintf(lg,"# W1c null_es %d (random update signs: fitness ignored)\n",null_es);
    if(hist_pair)std::fprintf(lg,"# W1e hist_pair %d (an ancestor drawn for each fork pair)\n",hist_pair);
    if(!archive.empty())lin[0].g->save(archive+"/egg_0.bin",0);}
  else std::fprintf(lg,"# resumed at epoch %d, threads %d\n",e0,threads);
  for(int e=e0;e<epochs;++e){
    const auto t0=std::chrono::steady_clock::now();
    if(wp.season>0&&e>0&&e%climate_every==0)canon.set_climate((std::uint64_t)(e/climate_every));  // new climate (forks copy it)
    int k0=-1;for(int k=0;k<Kmax;++k)if(lin[k].active){k0=k;break;}  // one lineage is always active (reseeded if all die)
    for(int k=0;k<Kmax;++k)if(lin[k].active)lin[k].nz.gen(*lin[k].g,P,lin[k].id==0?seed:mixu(seed*31+lin[k].id),e);
    const int F2=2*P;std::vector<std::vector<long>> fit(Kmax,std::vector<long>(F2,0));
    const evo::Egg* an=(hist&&!anc.empty())?anc[mixu(seed^((std::uint64_t)e<<8))%anc.size()].get():nullptr;
    const int T=std::max(1,std::min(threads,F2));std::vector<std::thread> th;
    for(int ti=0;ti<T;++ti){const int f0=F2*ti/T,f1=F2*(ti+1)/T;
      th.emplace_back([&,f0,f1]{const int nw=f1-f0;std::vector<w::World> ws(nw,canon);evo::Rows r;r.init(*lin[k0].g,K*nw*wp.S);
        const bool hp=hist_pair&&!anc.empty();std::vector<const evo::Egg*> anp(nw,nullptr);  // W1e: an ancestor per pair
        if(hp)for(int i=0;i<nw;++i){const int p=(f0+i)/2;anp[i]=anc[mixu(seed^((std::uint64_t)e<<8)^((std::uint64_t)p<<40))%anc.size()].get();}
        for(int i=0;i<nw;++i){const int f=f0+i,p=f/2;
          if(hp||(an&&p%hist_frac==0))ws[i].reseed_lineage(Kh,hist_founders,(std::uint64_t)e*P+p);  // same placement in both forks of the pair
          for(int k=0;k<K;++k)for(int s=0;s<wp.S;++s){const int row=(k*nw+i)*wp.S+s;r.pair[row]=p;r.sign[row]=k<Kmax?((f%2==0)?1:-1):0;
            std::copy(rc.state.begin()+(std::size_t)(k*wp.S+s)*Ls,rc.state.begin()+(std::size_t)(k*wp.S+s+1)*Ls,r.state.begin()+(std::size_t)row*Ls);}}
        std::vector<std::unique_ptr<evo::Stepper>> own;std::vector<evo::Stepper*> st(K,nullptr);
        for(int k=0;k<Kmax;++k)if(lin[k].active){own.push_back(std::make_unique<evo::Stepper>(*lin[k].g,&lin[k].nz,r,k*nw*wp.S,nw*wp.S));st[k]=own.back().get();}
        std::vector<evo::Stepper*> pf;
        if(hp){pf.assign(nw,nullptr);for(int i=0;i<nw;++i){own.push_back(std::make_unique<evo::Stepper>(*anp[i],nullptr,r,(Kh*nw+i)*wp.S,wp.S));pf[i]=own.back().get();}st[Kh]=pf[0];}
        else if(an){own.push_back(std::make_unique<evo::Stepper>(*an,nullptr,r,Kh*nw*wp.S,nw*wp.S));st[Kh]=own.back().get();}
        std::vector<std::vector<long>> integ(K,std::vector<long>(nw,0));run_worlds(ws,r,st,wp.S,H,&integ,nullptr,hp?&pf:nullptr,Kh);
        for(int k=0;k<Kmax;++k)for(int i=0;i<nw;++i)fit[k][f0+i]=integ[k][i]/H;});}
    for(auto& x:th)x.join();
    long ch=0;double alpha=0,m=0,m2=0;
    for(int f=0;f<F2;++f){long s=0;for(int k=0;k<Kmax;++k)if(lin[k].active)s+=fit[k][f];m+=s;m2+=(double)s*s;}
    for(int k=0;k<Kmax;++k){if(!lin[k].active)continue;std::vector<int> F(P);
      for(int p=0;p<P;++p){F[p]=fit[k][2*p]>fit[k][2*p+1]?1:fit[k][2*p]<fit[k][2*p+1]?-1:0;}
      if(null_es)for(int p=0;p<P;++p)F[p]=(mixu(mixu(seed^0x4e11ULL)+((std::uint64_t)e<<24)+((std::uint64_t)lin[k].id<<12)+p)&1)?1:-1;  // W1c: fitness ignored
      const double al=std::max(alpha_floor,1.0/(0.015*lin[k].age+1));if(k==k0)alpha=al;ch+=evo::egg_update(*lin[k].g,lin[k].nz,F,al);++lin[k].age;}
    m/=F2;const double sd=std::sqrt(std::max(0.0,m2/F2-m*m));
    {std::vector<w::World> one{canon};std::vector<std::unique_ptr<evo::Stepper>> own;std::vector<evo::Stepper*> st(K,nullptr);
      for(int k=0;k<Kmax;++k)if(lin[k].active){own.push_back(std::make_unique<evo::Stepper>(*lin[k].g,nullptr,rc,k*wp.S,wp.S));st[k]=own.back().get();}
      run_worlds(one,rc,st,wp.S,H,nullptr);canon=one[0];}
    // lineage dynamics: extinction (or reseed of the first active lineage if nothing is left), then splits
    int alive_l=0;for(int k=0;k<Kmax;++k)if(lin[k].active&&canon.count(k)>0)++alive_l;
    for(int k=0;k<Kmax;++k){if(!lin[k].active||canon.count(k)>0)continue;
      if(alive_l==0&&k==k0){canon.reseed_lineage(k,founders,(std::uint64_t)e+1);std::fill(rc.state.begin(),rc.state.end(),0);++reseeds;continue;}
      if(!archive.empty())lin[k].g->save(archive+"/extinct_L"+std::to_string(lin[k].id)+"_e"+std::to_string(e+1)+".bin",e+1);
      lin[k].active=false;lin[k].g.reset();++extinct;}
    for(int k=0;k<Kmax;++k){if(!lin[k].active||canon.count(k)<split_at)continue;int j=-1;for(int q=0;q<Kmax;++q)if(!lin[q].active){j=q;break;}if(j<0)break;
      lin[j].g=std::make_unique<evo::Egg>(bc,seed+next_lid);copy_weights(*lin[j].g,*lin[k].g);
      std::uint64_t c=0;for(auto* M:lin[j].g->mats)for(auto& x:M->w){const std::uint64_t h=mixu(mixu(seed^0x5b1ULL)+(next_lid<<40)+c++);if(h%10==0)x=(std::int8_t)std::clamp((int)x+((h>>8)&1?1:-1),-127,127);}
      lin[j].g->repack();
      lin[j].active=true;lin[j].id=next_lid++;lin[j].age=split_age?lin[k].age:0;++splits;int moved=0,t=0;
      for(int s=0;s<wp.S;++s){if(!canon.ag[(std::size_t)k*wp.S+s].alive)continue;if((moved++)%2==0)continue;
        while(t<wp.S&&canon.ag[(std::size_t)j*wp.S+t].alive){++t;}
        if(t>=wp.S)break;
        canon.move_slot(k*wp.S+s,j*wp.S+t);
        std::copy(rc.state.begin()+(std::size_t)(k*wp.S+s)*Ls,rc.state.begin()+(std::size_t)(k*wp.S+s+1)*Ls,rc.state.begin()+(std::size_t)(j*wp.S+t)*Ls);}}
    // champion (most abundant lineage): archived for tournaments and historical self-play
    int ck=-1;for(int k=0;k<Kmax;++k)if(lin[k].active&&(ck<0||canon.count(k)>canon.count(ck)))ck=k;
    const double sec=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
    long cb=0;int cp=0,na=0;for(int k=0;k<Kmax;++k){cb+=canon.biomass(k);cp+=canon.count(k);na+=lin[k].active;}
    if(Kmax==1)std::fprintf(lg,"%d\t%d\t%ld\t%.1f\t%.1f\t%ld\t%.4f\t%.2f\t%d\t%ld\n",e+1,cp,cb,m,sd,ch,alpha,sec,reseeds,canon.resources());
    else{std::string cs;for(int k=0;k<Kmax;++k)if(lin[k].active)cs+=(cs.empty()?"":",")+std::to_string(lin[k].id)+":"+std::to_string(canon.count(k));
      if(!wp.matter)std::fprintf(lg,"%d\t%d\t%ld\t%.1f\t%.1f\t%ld\t%.4f\t%.2f\t%d\t%ld\t%d\t%d\t%d\t%s\n",e+1,cp,cb,m,sd,ch,alpha,sec,reseeds,canon.resources(),na,splits,extinct,cs.c_str());
      else{int nm,nwl,nt,nc;canon.census(100,nm,nwl,nt,nc);  // W3: machines, walls, distinct tapes and effect classes (machines >= 100 ticks old)
        std::fprintf(lg,"%d\t%d\t%ld\t%.1f\t%.1f\t%ld\t%.4f\t%.2f\t%d\t%ld\t%d\t%d\t%d\t%s\t%d\t%d\t%d\t%d\n",e+1,cp,cb,m,sd,ch,alpha,sec,reseeds,canon.resources(),na,splits,extinct,cs.c_str(),nm,nwl,nt,nc);}}
    std::fflush(lg);
    if(ck>=0&&!archive.empty()&&((e+1)%archive_every==0||e+1==epochs))lin[ck].g->save(archive+"/egg_"+std::to_string(e+1)+".bin",e+1);
    if(ck>=0&&hist&&(e+1)%archive_every==0){auto cpy=std::make_unique<evo::Egg>(bc,1);copy_weights(*cpy,*lin[ck].g);anc.push_back(std::move(cpy));if(anc.size()>32)anc.erase(anc.begin());}
    if(!ckf.empty()&&((e+1)%ck_every==0||e+1==epochs)&&!save_ck(e+1)){std::fprintf(stderr,"cannot write checkpoint %s\n",ckf.c_str());return 3;}
  }
  if(lg!=stdout)std::fclose(lg);
}
