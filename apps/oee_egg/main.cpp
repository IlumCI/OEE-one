// EGG pretraining with integer EGGROLL on a byte corpus (docs/EVO.md).
// Usage: oee_egg -data FILE [-pop 1024] [-seqs 16] [-tokens 100] [-steps 1000] [-seed 1] [-threads 4]
//   [-L 6] [-D 256] [-sig 4] [-eval_every 50] [-eval_streams 32] [-eval_tokens 256] [-log FILE] [-save CK] [-resume CK]
// Data split (enwik8 convention): first 90% train, next 5% validation, last 5% test.
// Rows: R = max(pop, 2*seqs); row r belongs to pair (r/2) % (pop/2) with sign + (even) or - (odd) and reads sequence
// (r/2) % seqs, so both members of a pair see the same bytes. Hidden states persist across steps (truncated ES).
// Log: step bytes_seen train_bpb valid_bpb changed alpha sec macs_total
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "evo/egg.hpp"

using namespace oee::evo;

int main(int argc,char** argv){
  std::string data,logf,save,resume;int pop=1024,seqs=16,T=100,steps=1000,threads=4,eve=50,evs=32,evt=256;std::uint64_t seed=1;EggCfg c;
  for(int a=1;a<argc;a+=2){const char* s=argv[a];const char* v=a+1<argc?argv[a+1]:"";
    if(!std::strcmp(s,"-data"))data=v;else if(!std::strcmp(s,"-pop"))pop=std::atoi(v);else if(!std::strcmp(s,"-seqs"))seqs=std::atoi(v);
    else if(!std::strcmp(s,"-tokens"))T=std::atoi(v);else if(!std::strcmp(s,"-steps"))steps=std::atoi(v);else if(!std::strcmp(s,"-seed"))seed=std::strtoull(v,nullptr,10);
    else if(!std::strcmp(s,"-threads"))threads=std::atoi(v);else if(!std::strcmp(s,"-L"))c.L=std::atoi(v);else if(!std::strcmp(s,"-D"))c.D=std::atoi(v);
    else if(!std::strcmp(s,"-sig"))c.sig=std::atoi(v);else if(!std::strcmp(s,"-eval_every"))eve=std::atoi(v);else if(!std::strcmp(s,"-eval_streams"))evs=std::atoi(v);
    else if(!std::strcmp(s,"-eval_tokens"))evt=std::atoi(v);else if(!std::strcmp(s,"-log"))logf=v;else if(!std::strcmp(s,"-save"))save=v;else if(!std::strcmp(s,"-resume"))resume=v;
    else{std::fprintf(stderr,"unknown option %s\n",s);return 2;}}
  if(data.empty()||pop<2||pop%2){std::fprintf(stderr,"need -data and an even -pop\n");return 2;}
  std::FILE* df=std::fopen(data.c_str(),"rb");if(!df){std::fprintf(stderr,"cannot open %s\n",data.c_str());return 2;}
  std::vector<std::uint8_t> bytes;{std::uint8_t buf[1<<16];std::size_t n;while((n=std::fread(buf,1,sizeof buf,df))>0)bytes.insert(bytes.end(),buf,buf+n);std::fclose(df);}
  const std::size_t N=bytes.size(),ntr=N/10*9,nva=N/20;const std::uint8_t* tr=bytes.data();const std::uint8_t* va=bytes.data()+ntr;
  Egg g(c,seed);long step0=0;if(!resume.empty())step0=g.load(resume);
  std::FILE* lg=logf.empty()?stdout:std::fopen(logf.c_str(),step0?"a":"w");if(!lg)return 2;
  const int P=pop/2,R=std::max(pop,2*seqs);
  std::fprintf(lg,"# EGG L%d D%d sig%d params %ld pop %d pairs %d rows %d seqs %d tokens %d threads %d seed %llu macs/token %.0f\n",
    c.L,c.D,c.sig,g.nparams(),pop,P,R,seqs,T,threads,(unsigned long long)seed,egg_macs_per_token(c));std::fflush(lg);
  Rows rows;rows.init(g,R);for(int r=0;r<R;++r){rows.pair[r]=(r/2)%P;rows.sign[r]=r%2?-1:1;}
  std::vector<std::size_t> pos(seqs);for(int q=0;q<seqs;++q)pos[q]=(std::size_t)q*(ntr/seqs)+(std::size_t)step0*T;
  std::vector<std::uint8_t> tok((std::size_t)R*(T+1));std::vector<std::int64_t> fit(R);Noise nz;
  double macs=0;const double mpt=egg_macs_per_token(c);
  auto eval=[&]{Rows er;er.init(g,evs);std::vector<std::uint8_t> et((std::size_t)evs*(evt+1));
    for(int s=0;s<evs;++s){const std::size_t p0=(std::size_t)s*((nva-evt-1)/evs);std::memcpy(et.data()+(std::size_t)s*(evt+1),va+p0,evt+1);}
    std::vector<std::int64_t> ef(evs,0);run_rows(g,nullptr,er,et.data(),evt,ef.data(),threads);double s=0;for(auto x:ef)s+=x;return -s/16.0/((double)evs*evt);};
  for(long st=step0;st<step0+steps;++st){
    const auto t0=std::chrono::steady_clock::now();
    nz.gen(g,P,seed,st);
    for(int r=0;r<R;++r){const int q=(r/2)%seqs;std::size_t p=pos[q]%(ntr-T-1);std::memcpy(tok.data()+(std::size_t)r*(T+1),tr+p,T+1);}
    std::fill(fit.begin(),fit.end(),0);run_rows(g,&nz,rows,tok.data(),T,fit.data(),threads);macs+=mpt*R*T;
    std::vector<std::int64_t> sp(P,0),sm(P,0);double fs=0;for(int r=0;r<R;++r){(rows.sign[r]>0?sp:sm)[rows.pair[r]]+=fit[r];fs+=fit[r];}
    std::vector<int> F(P);for(int p=0;p<P;++p)F[p]=sp[p]>sm[p]?1:sp[p]<sm[p]?-1:0;
    const double alpha=1.0/(0.015*st+1);const long ch=egg_update(g,nz,F,alpha);
    for(auto& p:pos)p+=T;
    const double sec=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
    const bool ev=eve>0&&((st+1)%eve==0||st+1==step0+steps);
    std::fprintf(lg,"%ld\t%ld\t%.4f\t%s\t%ld\t%.5f\t%.2f\t%.4g\n",st+1,(long)(st+1)*seqs*T,-fs/16.0/((double)R*T),ev?std::to_string(eval()).substr(0,6).c_str():"-",ch,alpha,sec,macs);
    std::fflush(lg);
    if(!save.empty()&&ev)g.save(save,st+1);
  }
  if(lg!=stdout)std::fclose(lg);
}
