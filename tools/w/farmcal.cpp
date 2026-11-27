// W4e economics calibration (docs/WORLD.md, W4e protocol): food grown on a homestead farm's land over one epoch when
// the farm is kept fed (store topped up by 20 whenever it falls below 20, counted as energy fed) against the same
// farm left unfed. Food is counted as if harvested every tick. Agents idle. World flags as in the W4e runs.
// Usage: farmcal [irr_decay=1] [farm_store=40] [ticks=600] [irr=8] [irr_full=0] [farm_free=0] [farm_reset=1] [t0=0]
// (food is counted from tick t0 on; with farm_reset 0 the farms are placed once, at tick 0)
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "w/world.hpp"
using namespace oee::w;
int main(int argc,char** argv){
  const int dec=argc>1?std::atoi(argv[1]):1,fs=argc>2?std::atoi(argv[2]):40,T=argc>3?std::atoi(argv[3]):600,ir=argc>4?std::atoi(argv[4]):8,
    full=argc>5?std::atoi(argv[5]):0,fre=argc>6?std::atoi(argv[6]):0,rst=argc>7?std::atoi(argv[7]):1,t0=argc>8?std::atoi(argv[8]):0;
  for(int sd=1;sd<=3;++sd){double prod[2]={0,0},en=0,ilv[2]={0,0};
    for(int fed=0;fed<2;++fed){WParams p;p.L=32;p.S=64;p.K=4;p.regrow=16;p.nact=10;p.matter=1;p.spawn=20;p.mut=1024;p.season=300;p.view=1;
      p.signals=1;p.wall_decay=4;p.m_cost=4;p.m_give=0;p.graze=1;p.pass=1;p.irr=ir;p.irr_decay=dec;p.irr_full=full;p.farm_free=fre;p.farm_reset=rst;p.own=2;p.land=1;p.farms=6;p.farm_store=fs;
      p.farm_every=1<<30;p.stoch=0;p.seed=(std::uint64_t)sd;World w;w.init(p,16);
      std::vector<std::int8_t> l((std::size_t)p.S*w.nv(),(std::int8_t)-100);for(int s=0;s<p.S;++s)l[(std::size_t)s*w.nv()]=100;
      std::vector<const std::int8_t*> ls(p.K,l.data());std::vector<int> bn;
      for(int t=0;t<t0+T;++t){if(t>=std::max(1,t0)&&fed)for(int i=0;i<p.farms;++i){const int c=w.home_cell(0,i);if(w.mat[c]==1&&w.mst[c]<20){w.mst[c]+=20;en+=20;}}
        bn.clear();w.tick(ls.data(),bn);
        for(int i=0;i<p.farms;++i){const int c=w.home_cell(0,i);if(t>=t0)prod[fed]+=w.res[c];w.res[c]=0;}}
      for(int i=0;i<p.farms;++i)ilv[fed]+=w.irl[w.home_cell(0,i)]/(double)p.farms;}
    std::printf("seed %d: food on lineage 0's homestead land over %d ticks: unfed %.0f, fed %.0f; energy fed %.0f; extra food per unit fed %.2f; mean irrigation at the end unfed %.0f, fed %.0f\n",
      sd,T,prod[0],prod[1],en,en>0?(prod[1]-prod[0])/en:0.0,ilv[0],ilv[1]);}
  return 0;}
