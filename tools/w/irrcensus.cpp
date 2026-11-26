// W4a census of a checkpoint: machines (running, with I, E, both, C) and irrigated cells. Usage: irrcensus CK
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <algorithm>
#include "w/world.hpp"
using namespace oee::w;
int main(int argc,char** argv){std::FILE* f=std::fopen(argv[1],"rb");std::int64_t x,n;std::fread(&x,8,1,f);std::fread(&n,8,1,f);std::fseek(f,(long)(8*n+40),SEEK_CUR);World W;if(!W.load(f))return 2;
  const int N=W.p.L*W.p.L;int m=0,withI=0,withE=0,withIE=0,withC=0,run=0;long irsum=0;int irpos=0;
  for(int c=0;c<N;++c){if(W.p.irr&&W.irl[c]>0){++irpos;irsum+=W.irl[c];}if(W.mat[c]!=1)continue;++m;bool I=false,E=false,C=false;for(int j=0;j<16;++j){auto b=W.tape[c*16+j];I|=b==',';E|=b=='E';C|=b=='C';}
    withI+=I;withE+=E;withIE+=I&&E;withC+=C;run+=W.mst[c]>0;}
  std::printf("machines %d running %d withI %d withE %d withIE %d withC %d | irrigated cells %d mean irr %.1f\n",m,run,withI,withE,withIE,withC,irpos,irpos?(double)irsum/irpos:0.0);
  long fed=0;int alive=0;for(auto& a:W.ag)if(a.alive)++alive;std::printf("agents %d\n",alive);
  if(W.p.land&&!W.lown.empty()){std::vector<int> own(W.p.K+1,0);for(int c=0;c<N;++c)if(W.irl[c]>0)++own[W.lown[c]>=0?W.lown[c]+1:0];
    std::printf("land (irrigated cells by owner lineage; common first): common %d",own[0]);for(int k=0;k<W.p.K;++k)std::printf(" L%d %d",k,own[k+1]);std::printf("\n");}
  return 0;}
