// Do Geb agents use their signalling channel? Replays a checkpoint and records, per agent per step,
// what it heard and what it did.
// Usage: signals CHECKPOINT STEPS [deaf 0|1|2] [freeze 0|1] [reseed S] > out.tsv   (2: random half of agents deaf, by id;
//   freeze 1: lifetime plasticity off, weights kept as they are; docs/PLASTIC.md)
// Heard: 3 bits, one per signal class with a non-zero input (inputs listening for reproduce '01',
//   attack '1..', bond/unbond '00..'; an input listens for neighbour outputs starting with its
//   character minus the side bit, so the class is that remainder's action family).
// Did: 5 bits from the action sums the agent's own outputs produce (as in Organism::actions):
//   reproduce>500, attack>500, move>0, turn (|left-right|>0), bond>500. An action with no matching
//   output node is drawn at random by Geb; that is recorded as 0 here.
// Heard bit 3: an input whose remainder is compatible with the signal prefix '0011' is non-zero.
// Did bit 5: own signal output ('0011', plastic bit 1) is non-zero.
// Output rows: t heard did deafhalf has_inputs. Footer: population, births and colony stats per step on stderr.
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "geb/serial.hpp"
#include "geb/world.hpp"

using namespace oee::geb;

namespace {
int sum_prefix(const Ann& a,const char* bits){
  std::string p(bits);long s=0;bool any=false;
  for(const Ann::OutCache& o:a.output_cache()){
    const GeneSpan& c=o.node->character;if(c.length()/2<(int)p.size())continue;
    bool m=true;for(std::size_t k=0;k<p.size();++k)if(c.at(2*(int)k+1)!=(p[k]=='1')){m=false;break;}
    if(m){s+=o.io_value;any=true;}
  }
  return any?(int)std::min<long>(s,1024):-1;
}
bool hears_sig(const GeneSpan& c){const char* q="0011";const int nb=c.length()/2-1;for(int k=0;k<nb&&k<4;++k)if(c.at(2*(k+1)+1)!=(q[k]=='1'))return false;return true;}
int listen_class(const GeneSpan& c){  // remainder after the side bit
  const int nb=c.length()/2;if(nb<2)return -1;
  const int b1=c.at(3);if(b1==1)return 1;           // '1..' attack / turn / move family
  if(nb<3)return -1;const int b2=c.at(5);return b2?0:2;  // '01' reproduce, '00' bond/unbond
}
}  // namespace

int main(int argc,char** argv){
  if(argc<3)return 2;
  std::FILE* f=std::fopen(argv[1],"rb");if(!f)return 2;
  std::vector<unsigned char> b;unsigned char buf[1<<16];std::size_t n;
  while((n=std::fread(buf,1,sizeof buf,f))>0)b.insert(b.end(),buf,buf+n);
  std::fclose(f);b.resize(b.size()-8);
  Reader r(b.data(),b.size());r.get<std::uint64_t>();r.get_str();
  auto w=World::load(r);
  const long steps=std::atol(argv[2]);const int deaf=argc>3?std::atoi(argv[3]):0;w->set_deaf(deaf);Ann::freeze=argc>4&&std::atoi(argv[4]);if(argc>5)w->reseed(std::atoi(argv[5]));
  const long end=w->time()+steps;long births0=w->num_reproductions();
  while(w->time()<end){
    const long t0=w->time();w->step();if(w->time()==t0)continue;
    int pop=0,warriors=0;
    for(int c=0;c<w->world_size();++c){
      const Organism* o=w->at(c);if(!o)continue;++pop;warriors+=o->warrior();
      int heard=0;
      for(const IoEntry& in:const_cast<Organism*>(o)->net_inputs()){if(!in.io_value)continue;const int k=listen_class(in.node->character);if(k>=0)heard|=1<<k;if(hears_sig(in.node->character))heard|=8;}
      const int rep=sum_prefix(*o,"01"),kil=sum_prefix(*o,"100"),tl=sum_prefix(*o,"101"),tr=sum_prefix(*o,"110"),mv=sum_prefix(*o,"111"),bd=sum_prefix(*o,"000");
      const int did=(rep>500)|((kil>500)<<1)|((mv>0)<<2)|((std::abs(std::max(tl,0)-std::max(tr,0))>0)<<3)|((bd>500)<<4)|((sum_prefix(*o,"0011")>0)<<5);
      int nin=0;for(const IoEntry& in:const_cast<Organism*>(o)->net_inputs()){(void)in;++nin;}
      std::printf("%ld\t%d\t%d\t%d\t%d\n",w->time(),heard,did,(int)World::deaf_half(o->id),nin>0);
    }
    std::fprintf(stderr,"%ld\t%d\t%ld\t%d\n",w->time(),pop,w->num_reproductions()-births0,warriors);
  }
}
