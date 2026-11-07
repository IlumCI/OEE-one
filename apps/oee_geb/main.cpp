// Headless Geb driver. Reproduces the output of the original geb binary run without
// a display (docs/GEB_SPEC.md section 8), plus optional OEE-one extensions.
//
// Original options: -seed N -length N -neurons N -stats -rulestat -stopat N
// Extensions:       -variant resets|no_resets   (default set at compile time)
//                   -components FILE            (component dictionary for analysis)
//                   -outdir DIR                 (where rulestat files are written)

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

#include "geb/rulestat.hpp"
#include "geb/world.hpp"

#ifndef OEE_GEB_DEFAULT_RESETS
#define OEE_GEB_DEFAULT_RESETS 1
#endif

using namespace oee::geb;

namespace {

constexpr int kStatsEvery = 100;
constexpr int kRulestatEvery = 1000;

struct FileCloser {
  void operator()(std::FILE* f) const {
    if (f) std::fclose(f);
  }
};
using File = std::unique_ptr<std::FILE, FileCloser>;

File open_or_die(const std::string& path) {
  std::FILE* f = std::fopen(path.c_str(), "w");
  if (!f) {
    std::fprintf(stderr, "Failed to open file %s for writing\n", path.c_str());
    std::exit(1);
  }
  return File(f);
}

void stats(const World& w) {
  int age_total = 0, num = 0, youngest = static_cast<int>(w.time()), oldest = 0;
  long total_len = 0, shadow_total_len = 0;
  int pens[5] = {0, 0, 0, 0, 0};
  for (int at = 0; at < w.world_size(); ++at) {
    const Organism* o = w.at(at);
    if (o) {
      ++num;
      age_total += o->age;
      if (o->age > oldest) oldest = o->age;
      if (o->age < youngest) youngest = o->age;
      ++pens[o->pen_colour + 1];
      total_len += static_cast<long>(o->genotype().size());
    }
  }
  for (int k = 0; k < w.shadow_count(); ++k)
    shadow_total_len += static_cast<long>(w.shadow_genotype(k).size());
  std::fprintf(stdout,
               "time: T%ldT\taverage_age: A%fA\toldest: O%dO\tyoungest: Y%dY number: N%dN\t"
               "number of reproductions: R%ldR\tblacK%dK\tB%dBlue\treD%dD\tG%dGreen\n",
               w.time(), static_cast<double>(age_total) / num, oldest, youngest, num,
               w.num_reproductions(), pens[0], pens[2], pens[3], pens[4]);
  std::fprintf(stdout, "time: S%ldS\tSHADOWstats__num_individuals %d\n", w.time(),
               w.shadow_count());
  std::fprintf(stdout, "time: L%ldL\tmean_genotype_length %d SHADOWmean_genotype_length %d\n",
               w.time(), static_cast<int>(total_len / num),
               static_cast<int>(shadow_total_len / w.shadow_count()));
  std::fflush(stdout);
  if (num < 2) {
    std::printf("Terminating because number of individuals = %d\n", num);
    std::fflush(stdout);
    std::exit(108);
  }
}

}  // namespace

int main(int argc, char** argv) {
  std::fprintf(stderr, "Geb version 08b3. Copyright (C) Alastair Channon.\n");
  Params p;
  bool stats_on = false, rulestat_on = false, resets = OEE_GEB_DEFAULT_RESETS;
  long stopat = 0;
  std::string components_path, outdir = ".";
  for (int a = 1; a < argc;) {
    const char* s = argv[a];
    const bool has = (a + 1) < argc;
    if (!std::strcmp(s, "-neurons") && has && std::atol(argv[a + 1])) {
      p.max_num_nodes = static_cast<int>(std::atol(argv[a + 1]));
      std::fprintf(stdout, "Maximum number of neurons per agent set to %d\n", p.max_num_nodes);
      std::fflush(stdout);
      a += 2;
    } else if (!std::strcmp(s, "-length") && has && std::atol(argv[a + 1])) {
      p.world_length = static_cast<int>(std::atol(argv[a + 1]));
      a += 2;
    } else if (!std::strcmp(s, "-seed") && has && std::atol(argv[a + 1])) {
      p.seed = static_cast<int>(std::atol(argv[a + 1]));
      a += 2;
    } else if (!std::strcmp(s, "-stopat") && has && std::atol(argv[a + 1])) {
      stopat = std::atol(argv[a + 1]);
      a += 2;
    } else if (!std::strcmp(s, "-stats")) {
      stats_on = true;
      ++a;
    } else if (!std::strcmp(s, "-rulestat")) {
      rulestat_on = true;
      ++a;
    } else if (!std::strcmp(s, "-variant") && has) {
      resets = !std::strcmp(argv[a + 1], "resets");
      a += 2;
    } else if (!std::strcmp(s, "-components") && has) {
      components_path = argv[a + 1];
      a += 2;
    } else if (!std::strcmp(s, "-outdir") && has) {
      outdir = argv[a + 1];
      a += 2;
    } else {
      std::fprintf(stderr, "Unrecognised argument %s\n", s);
      return 2;
    }
  }
  std::fprintf(stderr, "\nCould not open display ");
  std::fprintf(stderr, "\nRunning without a display\n");
  std::fflush(stderr);

  World world(p);
  File rs, srs, rv, srv, dict;
  std::unique_ptr<RuleStat> rulestat;
  if (rulestat_on) {
    rs = open_or_die(outdir + "/rulestat");
    srs = open_or_die(outdir + "/SHADOWrulestat");
    rv = open_or_die(outdir + "/rulestat.verifier");
    srv = open_or_die(outdir + "/SHADOWrulestat.verifier");
    if (!components_path.empty()) dict = open_or_die(components_path);
    rulestat = std::make_unique<RuleStat>(resets, kRulestatEvery, rs.get(), srs.get(), rv.get(),
                                          srv.get(), stdout, dict.get());
    rulestat->write_headers();
  }

  bool done = false;
  while (!done) {
    const long before = world.time();
    world.step();
    if (world.time() != before) {
      const long t = world.time();
      if (stats_on && !(t % kStatsEvery)) stats(world);
      if (rulestat && !(t % kRulestatEvery)) rulestat->snapshot(world);
      if (stopat && t == stopat) done = true;
    }
  }
  world.terminate();
  std::fflush(stdout);
  if (dict) std::fflush(dict.get());
  std::fprintf(stderr, "\nProcess Terminated\n");
  std::fflush(stderr);
  return 100;
}
