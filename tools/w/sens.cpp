// Input sensitivity of a genome on recorded play (docs/WORLD.md, agent study). For records of an oee_w -probe file
// that follow the same agent's record of the previous tick, the brain is stepped again from the recorded state with
// the recorded observation (check: the recorded logits come back exactly), then with one observation entry set to
// each test value. Reports the mean action probabilities (P(a) ~ 2^(logit/16), as the world samples) per value: the
// partial dependence of the policy on that entry (Friedman 2001), on the states and situations the agent really met.
// Usage: sens EGG PROBE [-n 20000] [-in i:v1,v2,...]...   (default entries: food underfoot, food in front, agent in
//        front, material in front, energy, age, land, cue)
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "evo/egg.hpp"

using namespace oee;
static const char* AN[]={"noop","fwd","left","right","eat","repro","attack","write","make","feed"};
int main(int argc,char** argv){
  if(argc<3){std::fprintf(stderr,"usage: sens EGG PROBE [-n N] [-in i:v1,v2,...]...\n");return 2;}
  int nmax=20000;std::vector<std::pair<int,std::vector<int>>> sp;
  for(int a=3;a+1<argc;a+=2){if(!std::strcmp(argv[a],"-n"))nmax=std::atoi(argv[a+1]);
    else if(!std::strcmp(argv[a],"-in")){const std::string s=argv[a+1];const std::size_t c=s.find(':');std::vector<int> v;
      for(std::size_t i=c+1;i<s.size();){std::size_t j=s.find(',',i);if(j==std::string::npos)j=s.size();v.push_back(std::atoi(s.substr(i,j-i).c_str()));i=j+1;}
      sp.push_back({std::atoi(s.c_str()),v});}}
  std::FILE* f=std::fopen(argv[2],"rb");char mg[8];std::int32_t hd[8];
  if(!f||std::fread(mg,1,8,f)!=8||std::memcmp(mg,"WPRB0001",8)||std::fread(hd,4,8,f)!=8){std::fprintf(stderr,"cannot read probe %s\n",argv[2]);return 2;}
  const int Din=hd[0],V=hd[1],LD=hd[2],na=hd[3],RS=24+Din+V+LD;
  evo::EggCfg c0;{std::FILE* e=std::fopen(argv[1],"rb");std::uint64_t m=0;std::int32_t c[5];  // the genome's own sizes (v2 header)
    if(!e||std::fread(&m,8,1,e)!=1||m!=0x32474745ULL||std::fread(c,4,5,e)!=5){std::fprintf(stderr,"cannot read EGG v2 %s\n",argv[1]);return 2;}std::fclose(e);
    c0.L=c[0];c0.D=c[1];c0.V=c[2];c0.sig=c[3];c0.Din=c[4];}
  evo::Egg g(c0,1);g.load(argv[1]);
  if(g.c.Din!=Din||g.c.V!=V||g.c.L*g.c.D!=LD){std::fprintf(stderr,"egg and probe sizes differ\n");return 2;}
  if(sp.empty()){sp={{12,{0,2,4,8,16,32,64,127}},{7,{0,8,32,127}},{32,{-64,0,64}},{50,{0,4,8,16,32,64,125}},{51,{0,25,50,75,100,125}}};
    if(Din>=104)sp.push_back({86,{-48,0,48,96}});if(Din>=123)sp.push_back({122,{-64,0,64}});if(Din>=125){sp.push_back({123,{-64,-16,-8,0,8,16,64}});sp.push_back({124,{-64,-16,-8,0,8,16,64}});}}
  std::vector<std::uint8_t> all;{std::vector<std::uint8_t> b(1<<20);std::size_t n;while((n=std::fread(b.data(),1,b.size(),f))>0)all.insert(all.end(),b.begin(),b.begin()+n);}std::fclose(f);
  const std::size_t N=all.size()/RS;
  // pair each record with the same agent's record of the previous tick (game, id, t-1)
  std::map<std::uint64_t,std::size_t> last;std::vector<std::pair<std::size_t,std::size_t>> pr;
  for(std::size_t i=0;i<N;++i){const std::uint8_t* r=all.data()+i*RS;std::uint32_t t,id;std::memcpy(&t,r,4);std::memcpy(&id,r+4,4);
    const std::uint64_t k=((std::uint64_t)r[8]<<40)^((std::uint64_t)id<<1);auto it=last.find(k);
    if(it!=last.end()){std::uint32_t t0;std::memcpy(&t0,all.data()+it->second*RS,4);if(t0+1==t)pr.push_back({it->second,i});}
    last[k]=i;}
  if(pr.empty()){std::fprintf(stderr,"no consecutive records\n");return 2;}
  const std::size_t step=std::max<std::size_t>(1,pr.size()/(std::size_t)nmax);std::vector<std::pair<std::size_t,std::size_t>> use;
  for(std::size_t i=0;i<pr.size();i+=step)use.push_back(pr[i]);
  evo::Rows rows;rows.init(g,1);evo::Stepper st(g,nullptr,rows,0,1);std::vector<std::int8_t> ob(Din);
  auto run=[&](const std::uint8_t* prev,const std::int8_t* o){std::memcpy(rows.state.data(),prev+24+Din+V,LD);return st.step(o);};
  auto pol=[&](const std::int8_t* l,double* p){double m=-1e9;for(int a=0;a<na;++a)m=std::max(m,(double)l[a]);double s=0;
    for(int a=0;a<na;++a){p[a]=std::exp2((l[a]-m)/16.0);s+=p[a];}for(int a=0;a<na;++a)p[a]/=s;};
  long same=0;for(auto& u:use){const std::uint8_t* r=all.data()+u.second*RS;const std::int8_t* l=run(all.data()+u.first*RS,(const std::int8_t*)(r+24));
    same+=!std::memcmp(l,r+24+Din,V);}
  std::printf("records %zu, consecutive pairs %zu, used %zu; recorded logits reproduced in %ld of %zu\n",N,pr.size(),use.size(),same,use.size());
  std::printf("entry value  n");for(int a=0;a<na;++a)std::printf(" %7s",AN[a]);std::printf("  eat-fwd\n");
  for(auto& s:sp){const int i=s.first;if(i<0||i>=Din)continue;
    for(int v:s.second){double P[16]={0},d=0;double p[16];
      for(auto& u:use){const std::uint8_t* r=all.data()+u.second*RS;std::memcpy(ob.data(),r+24,Din);ob[i]=(std::int8_t)std::clamp(v,-128,127);
        const std::int8_t* l=run(all.data()+u.first*RS,ob.data());pol(l,p);for(int a=0;a<na;++a)P[a]+=p[a];d+=l[4]-l[1];}
      std::printf("%5d %5d %zu",i,v,use.size());for(int a=0;a<na;++a)std::printf(" %7.4f",P[a]/use.size());std::printf("  %7.2f\n",d/use.size());}}
  return 0;
}
