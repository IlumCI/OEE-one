// Machine-economy calibration (docs/WORLD.md, W3 calibration): worlds without agents.
// Usage: mcal [-m_cost 1] [-m_give 0] [-spawn 20] [-mut 1024] [-season 300] [-regrow 16] [-wall_decay 4] [-L 32]
//             [-ticks 20000] [-every 2000] [-seed 1] [-probe 0]
// -probe 1: one harvester (tape "E", store 50) on the most fertile cell, no spawn; prints its store every `every`.
// Otherwise spontaneous generation only; every `every` ticks prints: t machines walls tapes classes (census, age>=100)
// store (sum of machine stores) res (sum of cell resources) copiers (machines that made a copy) top (count of the most
// common tape among machines) and that tape (instruction bytes, '_' for no-op bytes).
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "w/world.hpp"

using namespace oee::w;
int main(int argc,char** argv){
  WParams p;p.K=1;p.S=16;p.nact=10;p.matter=1;p.spawn=20;p.mut=1024;p.season=300;p.view=1;p.signals=1;p.regrow=16;p.wall_decay=4;
  long T=20000,ev=2000;int probe=0;
  for(int a=1;a+1<argc;a+=2){const char* s=argv[a];const int v=std::atoi(argv[a+1]);
    if(!std::strcmp(s,"-m_cost"))p.m_cost=std::max(1,v);else if(!std::strcmp(s,"-m_give"))p.m_give=v;else if(!std::strcmp(s,"-spawn"))p.spawn=v;
    else if(!std::strcmp(s,"-mut"))p.mut=v;else if(!std::strcmp(s,"-season"))p.season=v;else if(!std::strcmp(s,"-regrow"))p.regrow=v;
    else if(!std::strcmp(s,"-wall_decay"))p.wall_decay=v;else if(!std::strcmp(s,"-L"))p.L=v;else if(!std::strcmp(s,"-ticks"))T=v;
    else if(!std::strcmp(s,"-every"))ev=std::max(1,v);else if(!std::strcmp(s,"-seed"))p.seed=(std::uint64_t)v;else if(!std::strcmp(s,"-probe"))probe=v;
    else{std::fprintf(stderr,"unknown flag %s\n",s);return 2;}}
  if(probe)p.spawn=0;
  World W;W.init(p,0);const int N=p.L*p.L;
  std::vector<std::int8_t> lg((std::size_t)p.S*W.nv(),0);const std::int8_t* l[1]={lg.data()};std::vector<int> born;
  int pc=-1;
  if(probe){int b=0;for(int c=0;c<N;++c)if(W.fert[c]>W.fert[b])b=c;pc=b;W.mat[pc]=1;W.mip[pc]=W.mh0[pc]=W.mh1[pc]=W.mdir[pc]=0;W.mhalt[pc]=0;W.mlin[pc]=-1;W.mbt[pc]=-1;W.mst[pc]=50;W.tape[(std::size_t)pc*16]='E';
    std::printf("# probe cell %d fert %d m_cost %d regrow %d season %d\nt\tstore\tres\thalted\n",pc,W.fert[pc],p.m_cost,p.regrow,p.season);}
  else std::printf("# m_cost %d m_give %d spawn %d mut %d season %d regrow %d wall_decay %d L %d seed %llu\nt\tmachines\twalls\ttapes\tclasses\tstore\tres\tcopiers\ttop\ttape\n",
    p.m_cost,p.m_give,p.spawn,p.mut,p.season,p.regrow,p.wall_decay,p.L,(unsigned long long)p.seed);
  for(long t=1;t<=T;++t){born.clear();W.tick(l,born);if(t%ev)continue;
    if(probe){std::printf("%ld\t%d\t%d\t%d\n",t,W.mat[pc]==1?W.mst[pc]:-1,W.res[pc],W.mat[pc]==1?W.mhalt[pc]:-1);continue;}
    int m=0,wl=0,tp=0,cl=0;W.census(100,m,wl,tp,cl);long st=0;int cp=0;std::map<std::string,int> h;
    for(int c=0;c<N;++c){if(W.mat[c]!=1)continue;st+=W.mst[c];cp+=W.mfx[(std::size_t)c*4+2]>0;std::string s(16,'_');
      for(int j=0;j<16;++j){const std::uint8_t x=W.tape[(std::size_t)c*16+j];if(std::find(kCode,kCode+16,x)!=kCode+16)s[j]=(char)x;}++h[s];}
    std::string best;int bn=0;for(auto& [k,v]:h)if(v>bn){bn=v;best=k;}
    std::printf("%ld\t%d\t%d\t%d\t%d\t%ld\t%ld\t%d\t%d\t%s\n",t,m,wl,tp,cl,st,W.resources(),cp,bn,best.c_str());}
  return 0;
}
