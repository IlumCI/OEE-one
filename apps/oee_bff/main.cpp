// BFF primordial soup driver. Log format matches cubff's --log output
// (epoch,brotli_size,soup_size,higher_entropy), followed by extra columns.
//
// Usage: oee_bff [-seed S] [-programs N] [-m5-protocol] [-grid W H (spatial variant, sets N = W*H)] [-mutation P (out of 2^30)] [-epochs E]
//                [-interval K] [-threads T] [-log FILE] [-cubff-log]
//                [-save CK] [-save-every K] [-resume CK] [-dump-every K -dump-from E -dump-programs N -dump-dir D]
//                [-stop-entropy X -stop-after K]
// -stop-entropy: stop K epochs after high-order entropy first reaches X (the paper's
// transition criterion is X = 1). Exit code 0 when stopped by -epochs, 10 when stopped
// after a transition.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>

#include "bff/soup.hpp"

using namespace oee::bff;

int main(int argc, char** argv) {
  Params p;
  std::size_t epochs = 16384, interval = 64, save_every = 0, dump_every = 0, dump_from = 0, dump_programs = 0;
  std::string log_path, save_path, resume_path, dump_dir;
  int sw_egc=-1,sw_guard=0;std::size_t keep_every=0;
  bool cubff_log = false;
  double stop_entropy = -1;
  std::size_t stop_after = 0, transition_epoch = 0;
  for (int a = 1; a < argc; ++a) {
    const std::string s = argv[a];
    auto next = [&]() -> const char* {
      if (a + 1 >= argc) { std::fprintf(stderr, "missing value for %s\n", s.c_str()); std::exit(2); }
      return argv[++a];
    };
    if (s == "-seed") p.seed = std::strtoull(next(), nullptr, 10);
    else if (s == "-programs") p.num_programs = std::strtoull(next(), nullptr, 10);
    else if (s == "-mutation") p.mutation_prob = static_cast<std::uint32_t>(std::strtoul(next(), nullptr, 10));
    else if (s == "-epochs") epochs = std::strtoull(next(), nullptr, 10);
    else if (s == "-interval") interval = std::strtoull(next(), nullptr, 10);
    else if (s == "-threads") p.threads = std::atoi(next());
    else if (s == "-m5-protocol") p.m5_protocol = true;
    else if(s=="-egc-std")p.egc_std=true;
    else if(s=="-switch-egc"){sw_egc=std::atoi(next());sw_guard=std::atoi(next());}
    else if(s=="-keep-every")keep_every=std::strtoull(next(),nullptr,10);  // also keep CK.<epoch> every K epochs
    else if(s=="-egc-guard")p.egc_guard=std::atoi(next());  // start every table at the standard code
    else if(s=="-egc")p.egc=std::atoi(next());  // 1: codon layout, fixed code; 2: evolvable code; 3, 4: external tables (option 2)
    else if(s=="-tab-shift")p.tab_shift=std::atoi(next());  // modes 3, 4: table mutation rate = mutation >> this
    else if (s == "-grid") { p.grid_w = std::atoi(next()); p.grid_h = std::atoi(next()); p.num_programs = std::size_t(p.grid_w) * p.grid_h; }
    else if (s == "-log") log_path = next();
    else if (s == "-cubff-log") cubff_log = true;
    else if (s == "-save") save_path = next();
    else if (s == "-save-every") save_every = std::strtoull(next(), nullptr, 10);
    else if (s == "-resume") resume_path = next();
    else if (s == "-dump-every") dump_every = std::strtoull(next(), nullptr, 10);
    else if (s == "-dump-dir") dump_dir = next();
    else if (s == "-dump-from") dump_from = std::strtoull(next(), nullptr, 10);
    else if (s == "-dump-programs") dump_programs = std::strtoull(next(), nullptr, 10);
    else if (s == "-stop-entropy") stop_entropy = std::atof(next());
    else if (s == "-stop-after") stop_after = std::strtoull(next(), nullptr, 10);
    else { std::fprintf(stderr, "unknown option %s\n", s.c_str()); return 2; }
  }
  Soup soup = resume_path.empty() ? Soup(p) : Soup::load(resume_path, p.threads);
  // -switch-egc M G: after resuming, change the code mode to M with guard G and the standard code in every table.
  if(sw_egc>=0){soup.set_egc(sw_egc,sw_guard,true);std::fprintf(stderr,"switched to egc %d guard %d at epoch %zu\n",sw_egc,sw_guard,soup.epochs_done());}
  std::FILE* log = log_path.empty() ? stdout : std::fopen(log_path.c_str(), soup.epochs_done() ? "a" : "w");
  if (!log) { std::perror("log"); return 1; }
  if (!soup.epochs_done())
    std::fprintf(log, cubff_log ? "epoch,brotli_size,soup_size,higher_entropy\n"
                                : soup.params().egc>=3?"epoch,brotli_size,soup_size,higher_entropy,h0,brotli_bpb,ops_per_run,tables,std_share,top_share,inherited\n"
                                : "epoch,brotli_size,soup_size,higher_entropy,h0,brotli_bpb,ops_per_run\n");
  // cubff: metrics after the epoch with index e when e % interval == 0, reported as e + 1;
  // stops once the reported epoch exceeds max_epochs.
  while (true) {
    const std::size_t e = soup.epochs_done();
    soup.epoch();
    if (e % interval == 0) {
      const Metrics m = soup.metrics();
      if (cubff_log)
        std::fprintf(log, "%zu,%zu,%zu,%f\n", m.epoch, m.brotli_size, soup.params().num_programs, m.higher_entropy);
      else
      if(soup.params().egc>=3){
      std::unordered_map<std::string,std::size_t> c;const auto&t=soup.tables();const std::size_t n=soup.params().num_programs;
      for(std::size_t i=0;i<n;++i)++c[std::string(reinterpret_cast<const char*>(&t[i*kCodon]),kCodon)];
      std::size_t top=0;for(auto&kv:c)top=std::max(top,kv.second);const auto it=c.find("[]+-.,<>{}");
      std::fprintf(log,"%zu,%zu,%zu,%f,%f,%f,%.2f,%zu,%f,%f,%zu\n",m.epoch,m.brotli_size,n,m.higher_entropy,m.h0,m.brotli_bpb,m.ops_per_run,
      c.size(),it==c.end()?0.0:double(it->second)/n,double(top)/n,soup.inherited());
      }else
        std::fprintf(log, "%zu,%zu,%zu,%f,%f,%f,%.2f\n", m.epoch, m.brotli_size, soup.params().num_programs,
                     m.higher_entropy, m.h0, m.brotli_bpb, m.ops_per_run);
      std::fflush(log);
      if (m.epoch > epochs) break;
      if (stop_entropy > 0 && !transition_epoch && m.higher_entropy >= stop_entropy) transition_epoch = m.epoch;
      if (transition_epoch && m.epoch >= transition_epoch + stop_after) {
        if (!save_path.empty()) soup.save(save_path);
        if (log != stdout) std::fclose(log);
        return 10;
      }
    }
    if (!save_path.empty() && save_every && soup.epochs_done() % save_every == 0) soup.save(save_path);
    if(!save_path.empty()&&keep_every&&soup.epochs_done()%keep_every==0)soup.save(save_path+"."+std::to_string(soup.epochs_done()));
    if (dump_every && !dump_dir.empty() && soup.epochs_done() >= dump_from && soup.epochs_done() % dump_every == 0) {
      char name[64];
      std::snprintf(name, sizeof name, "/soup_%010zu.bin", soup.epochs_done());
      std::FILE* f = std::fopen((dump_dir + name).c_str(), "wb");
      if (f) {
        const std::size_t n = dump_programs ? std::min(dump_programs * kTape, soup.data().size()) : soup.data().size();
        std::fwrite(soup.data().data(), 1, n, f);
        std::fclose(f);
      }
    }
  }
  if (!save_path.empty()) soup.save(save_path);
  if (log != stdout) std::fclose(log);
  return 0;
}
