// BFF primordial soup driver. Log format matches cubff's --log output
// (epoch,brotli_size,soup_size,higher_entropy), followed by extra columns.
//
// Usage: oee_bff [-seed S] [-programs N] [-mutation P (out of 2^30)] [-epochs E]
//                [-interval K] [-threads T] [-log FILE] [-cubff-log]
//                [-save CK] [-save-every K] [-resume CK] [-dump-every K -dump-dir D]
//                [-stop-entropy X -stop-after K]
// -stop-entropy: stop K epochs after high-order entropy first reaches X (the paper's
// transition criterion is X = 1). Exit code 0 when stopped by -epochs, 10 when stopped
// after a transition.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "bff/soup.hpp"

using namespace oee::bff;

int main(int argc, char** argv) {
  Params p;
  std::size_t epochs = 16384, interval = 64, save_every = 0, dump_every = 0;
  std::string log_path, save_path, resume_path, dump_dir;
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
    else if (s == "-log") log_path = next();
    else if (s == "-cubff-log") cubff_log = true;
    else if (s == "-save") save_path = next();
    else if (s == "-save-every") save_every = std::strtoull(next(), nullptr, 10);
    else if (s == "-resume") resume_path = next();
    else if (s == "-dump-every") dump_every = std::strtoull(next(), nullptr, 10);
    else if (s == "-dump-dir") dump_dir = next();
    else if (s == "-stop-entropy") stop_entropy = std::atof(next());
    else if (s == "-stop-after") stop_after = std::strtoull(next(), nullptr, 10);
    else { std::fprintf(stderr, "unknown option %s\n", s.c_str()); return 2; }
  }
  Soup soup = resume_path.empty() ? Soup(p) : Soup::load(resume_path, p.threads);
  std::FILE* log = log_path.empty() ? stdout : std::fopen(log_path.c_str(), soup.epochs_done() ? "a" : "w");
  if (!log) { std::perror("log"); return 1; }
  if (!soup.epochs_done())
    std::fprintf(log, cubff_log ? "epoch,brotli_size,soup_size,higher_entropy\n"
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
    if (dump_every && !dump_dir.empty() && soup.epochs_done() % dump_every == 0) {
      char name[64];
      std::snprintf(name, sizeof name, "/soup_%010zu.bin", soup.epochs_done());
      std::FILE* f = std::fopen((dump_dir + name).c_str(), "wb");
      if (f) { std::fwrite(soup.data().data(), 1, soup.data().size(), f); std::fclose(f); }
    }
  }
  if (!save_path.empty()) soup.save(save_path);
  if (log != stdout) std::fclose(log);
  return 0;
}
