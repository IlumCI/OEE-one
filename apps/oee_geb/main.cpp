// Headless Geb driver. Reproduces the output of the original geb binary run without
// a display (docs/GEB_SPEC.md section 8), plus optional OEE-one extensions.
//
// Original options: -seed N -length N -neurons N -stats -rulestat -stopat N
// Extensions:       -variant resets|no_resets   (default set at compile time)
//                   -components FILE            (component dictionary for analysis)
//                   -outdir DIR                 (where rulestat files are written)
//                   -random-selection           (GebR negative control)
//                   -second-shadow              (independent second reset shadow)
//                   -presence FILE              (hash-based component presence record)
//                   -log FILE                   (statistics output instead of stdout)
//                   -save FILE -save-every N    (checkpoint every N steps, at cycle end)
//                   -resume FILE                (continue from a checkpoint; output files are
//                                                truncated to their checkpointed lengths)

#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

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

std::FILE* out = stdout;

File open_for_resume(const std::string& path, long offset) {
  std::FILE* f = std::fopen(path.c_str(), "r+");
  if (!f || ftruncate(fileno(f), offset) != 0 || std::fseek(f, 0, SEEK_END) != 0) {
    std::fprintf(stderr, "Failed to reopen %s for resume\n", path.c_str());
    std::exit(1);
  }
  return File(f);
}

constexpr std::uint64_t kCheckpointMagic = 0x31544e504b434545ULL;  // "EECKPNT1"

struct Outputs {
  std::vector<std::pair<std::string, std::FILE*>> files;  // path, handle
};

void save_checkpoint(const std::string& path, const World& world, const RuleStat* rulestat,
                     const Outputs& outs, const std::string& config) {
  for (const auto& [name, f] : outs.files) std::fflush(f);
  Writer w;
  w.put(kCheckpointMagic);
  w.put_str(config);
  world.save(w);
  w.put<std::uint8_t>(rulestat ? 1 : 0);
  if (rulestat) rulestat->save(w);
  w.put<std::uint64_t>(outs.files.size());
  for (const auto& [name, f] : outs.files) {
    w.put_str(name);
    w.put<long>(std::ftell(f));
  }
  const auto& b = w.bytes();
  const std::uint64_t sum = fnv1a(b.data(), b.size());
  const std::string tmp = path + ".tmp";
  std::FILE* f = std::fopen(tmp.c_str(), "wb");
  if (!f) {
    std::fprintf(stderr, "Failed to write checkpoint %s\n", tmp.c_str());
    std::exit(1);
  }
  const bool ok = std::fwrite(b.data(), 1, b.size(), f) == b.size() &&
                  std::fwrite(&sum, sizeof sum, 1, f) == 1 && std::fflush(f) == 0 &&
                  fsync(fileno(f)) == 0;
  std::fclose(f);
  if (!ok) {
    std::fprintf(stderr, "Failed to write checkpoint %s\n", tmp.c_str());
    std::exit(1);
  }
  std::rename(path.c_str(), (path + ".prev").c_str());
  if (std::rename(tmp.c_str(), path.c_str()) != 0) {
    std::fprintf(stderr, "Failed to install checkpoint %s\n", path.c_str());
    std::exit(1);
  }
}

std::vector<unsigned char> read_checkpoint(const std::string& path) {
  std::FILE* f = std::fopen(path.c_str(), "rb");
  if (!f) throw std::runtime_error("cannot open checkpoint " + path);
  std::vector<unsigned char> b;
  unsigned char buf[1 << 16];
  std::size_t n;
  while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) b.insert(b.end(), buf, buf + n);
  std::fclose(f);
  if (b.size() < 8) throw std::runtime_error("checkpoint too short");
  std::uint64_t sum;
  std::memcpy(&sum, b.data() + b.size() - 8, 8);
  b.resize(b.size() - 8);
  if (fnv1a(b.data(), b.size()) != sum) throw std::runtime_error("checkpoint checksum mismatch");
  return b;
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
  std::fprintf(out,
               "time: T%ldT\taverage_age: A%fA\toldest: O%dO\tyoungest: Y%dY number: N%dN\t"
               "number of reproductions: R%ldR\tblacK%dK\tB%dBlue\treD%dD\tG%dGreen\n",
               w.time(), static_cast<double>(age_total) / num, oldest, youngest, num,
               w.num_reproductions(), pens[0], pens[2], pens[3], pens[4]);
  std::fprintf(out, "time: S%ldS\tSHADOWstats__num_individuals %d\n", w.time(),
               w.shadow_count());
  std::fprintf(out, "time: L%ldL\tmean_genotype_length %d SHADOWmean_genotype_length %d\n",
               w.time(), static_cast<int>(total_len / num),
               static_cast<int>(shadow_total_len / w.shadow_count()));
  std::fflush(out);
  if (num < 2) {
    std::fprintf(out, "Terminating because number of individuals = %d\n", num);
    std::fflush(out);
    std::exit(108);
  }
}

}  // namespace

int main(int argc, char** argv) {
  std::fprintf(stderr, "Geb version 08b3. Copyright (C) Alastair Channon.\n");
  Params p;
  bool stats_on = false, rulestat_on = false, resets = OEE_GEB_DEFAULT_RESETS;
  long stopat = 0;
  std::string components_path, presence_path, outdir = ".", log_path, save_path, resume_path;
  long save_every = 0;
  bool neurons_msg = false;
  for (int a = 1; a < argc;) {
    const char* s = argv[a];
    const bool has = (a + 1) < argc;
    if (!std::strcmp(s, "-neurons") && has && std::atol(argv[a + 1])) {
      p.max_num_nodes = static_cast<int>(std::atol(argv[a + 1]));
      neurons_msg = true;
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
    } else if (!std::strcmp(s, "-second-shadow")) {
      p.second_shadow = true;
      ++a;
    } else if (!std::strcmp(s, "-presence") && has) {
      presence_path = argv[a + 1];
      a += 2;
    } else if (!std::strcmp(s, "-random-selection")) {
      p.random_selection = true;
      ++a;
    } else if (!std::strcmp(s, "-log") && has) {
      log_path = argv[a + 1];
      a += 2;
    } else if (!std::strcmp(s, "-save") && has) {
      save_path = argv[a + 1];
      a += 2;
    } else if (!std::strcmp(s, "-save-every") && has && std::atol(argv[a + 1]) > 0) {
      save_every = std::atol(argv[a + 1]);
      a += 2;
    } else if (!std::strcmp(s, "-resume") && has) {
      resume_path = argv[a + 1];
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

  // Output configuration recorded in checkpoints; a resume must use the same one.
  const std::string config = std::string(stats_on ? "S" : "-") + (rulestat_on ? "R" : "-") +
                             (resets ? "r" : "n") + "|" + outdir + "|" + log_path + "|" +
                             components_path + "|" + presence_path;
  std::unique_ptr<World> world_ptr;
  std::vector<unsigned char> ckpt;
  std::unique_ptr<Reader> reader;
  std::vector<std::pair<std::string, long>> offsets;
  if (!resume_path.empty()) {
    try {
      ckpt = read_checkpoint(resume_path);
      reader = std::make_unique<Reader>(ckpt.data(), ckpt.size());
      if (reader->get<std::uint64_t>() != kCheckpointMagic) throw std::runtime_error("bad magic");
      if (reader->get_str() != config)
        throw std::runtime_error("output options differ from the checkpointed run");
      world_ptr = World::load(*reader);
    } catch (const std::exception& e) {
      std::fprintf(stderr, "Resume failed: %s\n", e.what());
      return 3;
    }
  } else {
    world_ptr = std::make_unique<World>(p);
  }
  World& world = *world_ptr;

  File log, rs, srs, rv, srv, dict, pres;
  std::unique_ptr<RuleStat> rulestat;
  Outputs outs;
  const bool resuming = reader != nullptr;
  std::uint8_t has_rulestat = 0;
  if (resuming) {
    // The RuleStat block precedes the offsets; read it after constructing RuleStat.
  }
  auto open_out = [&](const std::string& path) -> File {
    return resuming ? File(nullptr) : open_or_die(path);
  };
  std::vector<std::string> paths;
  if (!log_path.empty()) paths.push_back(log_path);
  if (rulestat_on) {
    paths.push_back(outdir + "/rulestat");
    paths.push_back(outdir + "/SHADOWrulestat");
    paths.push_back(outdir + "/rulestat.verifier");
    paths.push_back(outdir + "/SHADOWrulestat.verifier");
    if (!components_path.empty()) paths.push_back(components_path);
    if (!presence_path.empty()) paths.push_back(presence_path);
  }
  std::vector<File*> handles;
  if (!log_path.empty()) handles.push_back(&log);
  if (rulestat_on) {
    handles.push_back(&rs);
    handles.push_back(&srs);
    handles.push_back(&rv);
    handles.push_back(&srv);
    if (!components_path.empty()) handles.push_back(&dict);
    if (!presence_path.empty()) handles.push_back(&pres);
  }
  if (!resuming)
    for (std::size_t i = 0; i < paths.size(); ++i) *handles[i] = open_out(paths[i]);
  if (rulestat_on && resuming) {
    // Handles are opened below once offsets are known; RuleStat needs them now, so
    // construct it after reopening.
  }

  if (resuming) {
    has_rulestat = reader->get<std::uint8_t>();
    std::unique_ptr<RuleStat> tmp;
    if (has_rulestat) {
      tmp = std::make_unique<RuleStat>(resets, kRulestatEvery, nullptr, nullptr, nullptr, nullptr,
                                       nullptr, nullptr);
      tmp->load(*reader);
    }
    const auto nfiles = reader->get<std::uint64_t>();
    for (std::uint64_t i = 0; i < nfiles; ++i) {
      std::string name = reader->get_str();
      offsets.emplace_back(std::move(name), reader->get<long>());
    }
    if (offsets.size() != paths.size()) {
      std::fprintf(stderr, "Resume failed: output file set differs\n");
      return 3;
    }
    for (std::size_t i = 0; i < paths.size(); ++i) {
      if (offsets[i].first != paths[i]) {
        std::fprintf(stderr, "Resume failed: output path %s differs\n", paths[i].c_str());
        return 3;
      }
      *handles[i] = open_for_resume(paths[i], offsets[i].second);
    }
    rulestat = std::move(tmp);
  }
  if (log) out = log.get();
  if (!resuming && neurons_msg) {
    std::fprintf(out, "Maximum number of neurons per agent set to %d\n", p.max_num_nodes);
    std::fflush(out);
  }
  if (rulestat_on) {
    auto fresh = std::make_unique<RuleStat>(resets, kRulestatEvery, rs.get(), srs.get(), rv.get(),
                                            srv.get(), out, dict.get());
    if (rulestat) {
      // Transfer loaded component tables into the instance bound to the reopened files.
      Writer w;
      rulestat->save(w);
      Reader r(w.bytes().data(), w.bytes().size());
      fresh->load(r);
    } else {
      fresh->write_headers();
    }
    rulestat = std::move(fresh);
    if (pres) rulestat->set_presence_file(pres.get());
  }
  for (std::size_t i = 0; i < paths.size(); ++i) outs.files.emplace_back(paths[i], handles[i]->get());
  if (resuming)
    std::fprintf(stderr, "Resumed from %s at time %ld\n", resume_path.c_str(), world.time());

  bool done = false;
  while (!done) {
    const long before = world.time();
    world.step();
    if (world.time() != before) {
      const long t = world.time();
      if (stats_on && !(t % kStatsEvery)) stats(world);
      if (rulestat && !(t % kRulestatEvery)) rulestat->snapshot(world);
      if (!save_path.empty() && save_every && !(t % save_every))
        save_checkpoint(save_path, world, rulestat.get(), outs, config);
      if (stopat && t >= stopat) done = true;
    }
  }
  world.terminate();
  std::fflush(out);
  std::fflush(stdout);
  if (dict) std::fflush(dict.get());
  std::fprintf(stderr, "\nProcess Terminated\n");
  std::fflush(stderr);
  return 100;
}
