// Population tournament: does a later population out-compete an earlier one? (docs/TOURNEY.md)
// Usage: tourney CK_A CK_B STEPS [seed=1] [L=32] [n=128] [every=1000] [neurons=0]
// n genomes are sampled (by seed, without replacement) from the living agents of each checkpoint and placed
// alternately in the first 2n cells of a fresh L x L arena that uses B's rules (collectives, RNG mode,
// plasticity; locks off, no growth). All start as newborns developed from their genomes. neurons=0: the larger
// of the two neuron limits. Organism::anc marks ancestry: 1 = from A, 0 = from B; a child inherits the
// length-weighted share of its parents' genome segments. If CK_A and CK_B are the same file, the two groups
// are disjoint samples of one population (the drift control). Organism::mark is a uniparental marker (1 = A),
// copied from the randomly chosen first parent: its share stays 0.5 in expectation under neutrality, whatever the
// genome lengths (anc is biased toward the population with longer genomes).
// Output rows: t pop mean_anc frac_anc_gt_half mark_share births
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "geb/serial.hpp"
#include "geb/world.hpp"

using namespace oee::geb;

namespace {
std::unique_ptr<World> load(const char* fn){
  std::FILE* f=std::fopen(fn,"rb");if(!f){std::fprintf(stderr,"cannot open %s\n",fn);std::exit(2);}
  std::vector<unsigned char> b;unsigned char buf[1<<16];std::size_t n;
  while((n=std::fread(buf,1,sizeof buf,f))>0)b.insert(b.end(),buf,buf+n);
  std::fclose(f);b.resize(b.size()-8);
  Reader r(b.data(),b.size());r.get<std::uint64_t>();r.get_str();
  return World::load(r);
}
std::vector<Genotype> genomes(const World& w){std::vector<Genotype> g;for(int c=0;c<w.world_size();++c)if(const Organism* o=w.at(c))g.push_back(o->genotype());return g;}
}  // namespace

int main(int argc,char** argv){
  if(argc<4){std::fprintf(stderr,"usage: tourney CK_A CK_B STEPS [seed] [L] [n] [every] [neurons]\n");return 2;}
  const long T=std::atol(argv[3]);const int seed=argc>4?std::atoi(argv[4]):1,L=argc>5?std::atoi(argv[5]):32,
    n=argc>6?std::atoi(argv[6]):128,every=argc>7?std::atoi(argv[7]):1000,nn=argc>8?std::atoi(argv[8]):0;
  const bool same=!std::strcmp(argv[1],argv[2]);
  auto wa=load(argv[1]);auto wb=same?nullptr:load(argv[2]);const World& rb=same?*wa:*wb;
  std::vector<Genotype> ga=genomes(*wa),gb=same?std::vector<Genotype>():genomes(*wb);
  std::mt19937_64 rng(0x9e3779b97f4a7c15ULL*(std::uint64_t)seed+1);
  std::shuffle(ga.begin(),ga.end(),rng);if(!same)std::shuffle(gb.begin(),gb.end(),rng);
  if(same){if((int)ga.size()<2*n){std::fprintf(stderr,"population %zu < 2n\n",ga.size());return 2;}gb.assign(ga.begin()+n,ga.begin()+2*n);ga.resize(n);}
  else if((int)ga.size()<n||(int)gb.size()<n){std::fprintf(stderr,"population too small (%zu, %zu)\n",ga.size(),gb.size());return 2;}
  ga.resize(n);gb.resize(n);
  Params p=rb.params();
  p.world_length=L;p.max_num_nodes=nn>0?nn:std::max(wa->params().max_num_nodes,rb.params().max_num_nodes);
  p.start_num_individuals=2*n;p.max_num_individuals=-1;p.seed=seed;p.second_shadow=false;p.random_selection=false;
  p.collectives&=~128;  // arena without locks
  if(2*n>L*L){std::fprintf(stderr,"2n > L*L\n");return 2;}
  std::fprintf(stderr,"# A %s (%zu agents, neurons %d)  B %s (neurons %d)  arena L%d neurons %d collectives %d plastic %d seed %d\n",
    argv[1],genomes(*wa).size(),wa->params().max_num_nodes,argv[2],rb.params().max_num_nodes,L,p.max_num_nodes,p.collectives,p.plastic,seed);
  wa.reset();wb.reset();
  World w(p);
  std::vector<Genotype> pool;pool.reserve(2*n);for(int i=0;i<n;++i){pool.push_back(ga[i]);pool.push_back(gb[i]);}
  w.seed_genomes(pool);  // cell k (k < 2n) gets pool[k]: even cells A, odd cells B
  for(int c=0;c<2*n;++c)if(const Organism* o=w.at(c)){auto* q=const_cast<Organism*>(o);q->anc=c%2==0?1.f:0.f;q->mark=c%2==0;}
  const long b0=w.num_reproductions();
  auto row=[&](){double s=0;int k=0,m=0,pop=0;for(int c=0;c<w.world_size();++c)if(const Organism* o=w.at(c)){++pop;s+=o->anc;k+=o->anc>.5f;m+=o->mark;}
    std::printf("%ld\t%d\t%.4f\t%.4f\t%.4f\t%ld\n",w.time(),pop,pop?s/pop:0.0,pop?double(k)/pop:0.0,pop?double(m)/pop:0.0,w.num_reproductions()-b0);std::fflush(stdout);return pop;};
  row();
  while(w.time()<T){const long t0=w.time();w.step();if(w.time()!=t0&&w.time()%every==0&&row()<2)break;}
}
