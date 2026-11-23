// Dump the most complex agent of a checkpoint for visualisation: its production rules, its network
// after every development step (re-grown from the genotype, deterministic), and its live neuron
// activity over the next world steps.
// Usage: netdump CHECKPOINT [RANK=0] [STEPS=300] > out.json
// Complexity = used production rules (rulestat's used component count). RANK picks the k-th most complex.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <vector>

#include "geb/serial.hpp"
#include "geb/world.hpp"

using namespace oee::geb;

namespace {
std::string bits(const GeneSpan& s){std::string o;if(s.null())return o;for(int k=1;k<s.length();k+=2)o+=s.at(k)?'1':'0';return o;}
int used(const Ann& a){int n=0;for(const Rule& r:a.rules())n+=r.used;return n;}
void net(const Ann& a,std::unordered_map<const Node*,int>& id,int& nid){
  std::printf("{\"n\":[");bool f=true;
  for(const Node& x:a.nodes()){auto it=id.find(&x);if(it==id.end())it=id.emplace(&x,nid++).first;
    std::printf("%s[%d,\"%s\",%d,%d]",f?"":",",it->second,bits(x.character).c_str(),x.net_input?1:0,x.net_output?1:0);f=false;}
  std::printf("],\"l\":[");f=true;
  for(const Node& x:a.nodes()){
    for(const Node* y:x.excitatory_outputs){std::printf("%s[%d,%d,1]",f?"":",",id[&x],id[y]);f=false;}
    for(const Node* y:x.inhibitory_outputs){std::printf("%s[%d,%d,0]",f?"":",",id[&x],id[y]);f=false;}}
  std::printf("]}");
}
}  // namespace

int main(int argc,char** argv){
  if(argc<2)return 2;
  std::FILE* f=std::fopen(argv[1],"rb");if(!f)return 2;
  std::vector<unsigned char> b;unsigned char buf[1<<16];std::size_t n;
  while((n=std::fread(buf,1,sizeof buf,f))>0)b.insert(b.end(),buf,buf+n);
  std::fclose(f);b.resize(b.size()-8);
  Reader r(b.data(),b.size());r.get<std::uint64_t>();r.get_str();
  auto w=World::load(r);
  const int rank=argc>2?std::atoi(argv[2]):0;const long steps=argc>3?std::atol(argv[3]):300;
  std::vector<const Organism*> all;
  for(int c=0;c<w->world_size();++c)if(const Organism* o=w->at(c))all.push_back(o);
  std::sort(all.begin(),all.end(),[](const Organism* a,const Organism* b){return used(*a)!=used(*b)?used(*a)>used(*b):a->id<b->id;});
  if(rank>=(int)all.size())return 3;
  const Organism* h=all[rank];const std::uint64_t hid=h->id;
  // re-grow: find a node limit that reproduces the agent's network exactly
  int lim=-1;
  for(int L:{h->num_nodes(),20,40,60,84,100,116,132,148,164,188,256,512}){
    Ann a(h->genotype(),L);for(int s=0;s<kMaxMorphogenesisSteps;++s)a.morphogenesis_step();
    std::vector<std::string> p,q;for(const Node& x:a.nodes())p.push_back(bits(x.character));for(const Node& x:h->nodes())q.push_back(bits(x.character));
    if(p==q){lim=L;break;}
  }
  std::printf("{\"time\":%ld,\"L\":%d,\"population\":%zu,\"id\":%llu,\"age\":%d,\"used\":%d,\"nodes\":%d,\"limit\":%d,\"colony_member\":%d,",
    w->time(),w->world_length(),all.size(),(unsigned long long)hid,h->age,used(*h),h->num_nodes(),lim,(int)h->bonds.size());
  std::printf("\"rules\":[");bool fr=true;
  for(const Rule& x:h->rules()){std::printf("%s{\"pre\":\"%s\",\"rep\":\"%s\",\"new\":\"%s\",\"links\":\"%d%d%d%d%d%d\",\"used\":%d}",fr?"":",",
    bits(x.predecessor).c_str(),bits(x.successor_replacement).c_str(),bits(x.successor_new).c_str(),x.reverse_inputs_type_on_new!=0,x.reverse_outputs_type_on_new!=0,
    x.inhibitory_replacement_to_new!=0,x.excitatory_replacement_to_new!=0,x.inhibitory_new_to_replacement!=0,x.excitatory_new_to_replacement!=0,x.used?1:0);fr=false;}
  std::printf("],\"dev\":[");
  if(lim>0){
    Ann a(h->genotype(),lim);std::unordered_map<const Node*,int> id;int nid=0;std::string last;
    net(a,id,nid);
    for(int s=0;s<kMaxMorphogenesisSteps;++s){
      const int before=a.num_nodes();std::size_t nl=0;for(const Node& x:a.nodes())nl+=x.excitatory_outputs.size()+x.inhibitory_outputs.size();
      a.morphogenesis_step();
      std::size_t nl2=0;for(const Node& x:a.nodes())nl2+=x.excitatory_outputs.size()+x.inhibitory_outputs.size();
      if(a.num_nodes()==before&&nl==nl2)continue;   // record changes only
      std::printf(",");net(a,id,nid);
    }
  }
  // live activity of the real agent: per world step, per node (agent's own node order) excitatory - inhibitory output
  std::printf("],\"final_order\":[");
  {bool g=true;for(const Node& x:h->nodes()){std::printf("%s\"%s\"",g?"":",",bits(x.character).c_str());g=false;}}
  std::printf("],\"act\":[");
  bool fa=true;const long end=w->time()+steps;
  while(w->time()<end){
    const long t0=w->time();w->step();if(w->time()==t0)continue;
    const Organism* o=nullptr;for(int c=0;c<w->world_size()&&!o;++c){const Organism* q=w->at(c);if(q&&q->id==hid)o=q;}
    if(!o)break;
    std::printf("%s[",fa?"":",");fa=false;bool g=true;
    for(const Node& x:o->nodes()){std::printf("%s%d",g?"":",",x.excitatory_output-x.inhibitory_output);g=false;}
    std::printf("]");
  }
  std::printf("]}\n");
}
