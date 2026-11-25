# OEE-one plan: open-ended evolution toward ASI

Current version 2026-11-25. Sections 0–3 are binding. The original M1 plan is kept below as history.

## 0. Rules for every agent working in this repository

1. **Goal.** Capability that keeps growing without human data, produced by open-ended evolution, toward ASI.
   Success is measured as growth in capability, not novelty alone.
2. **No human knowledge as a training signal.**
   - No pretrained models of any kind inside the system or its loop: proprietary or open, large or small, local or
     remote.
   - Only models we evolve ourselves.
   - Reason (owner's thesis): models trained on human data inherit human limits. Getting past them requires the
     system's own experience in a world that keeps producing new challenges.
3. **Local compute only.** No paid or remote model APIs. The container is CPU-only (4 cores, Intel AMX int8 and bf16,
   no GPU, ever). Design for int8.
4. **Fitness comes only from the world.** That means survival, energy, reproduction, and competition with other
   evolved agents and archived ancestors. Never select on human benchmarks or human-written text.
5. **Measure progress without humans:**
   - ancestor tournaments (primary; tools/tourney, docs/TOURNEY.md);
   - held-out procedurally generated arenas;
   - ablations (freeze, mute, wipe);
   - observer compression progress (docs/WORLD.md §5).
   Every claim needs a control: the same-era drift band, null shadows, or paired seeds. With ±17-point drift on
   2 seeds, use at least 5 seeds.
6. **Fix a kill criterion before each experiment and write it in the doc first.** When progress flattens:
   - Diagnose what the world runs out of (resources, information, affordances) before adding a mechanism.
   - Do not keep adding mechanisms to the same substrate. That was the failure mode of the Geb ladder: L3 v4–v9,
     L4a/b, EGC 1–3, M5 v1–v3.
7. **The optimiser is evolution:** integer EGGROLL in src/evo, plus evolved lifetime plasticity. Gradient training is
   not part of the system.
8. **Before design decisions, consult arXiv** (CLAUDE.md), and record the papers used in the design doc.
9. **Stay at the frontier.** Current anchors:
   - Hughes et al. 2406.04268: open-endedness is novelty plus learnability.
   - Clune 1905.10985: AI-generating algorithms; evolve architecture, learning rule and environments.
   - EGGROLL 2511.16652: integer evolution strategies at scale.
   - POET 1901.01753: co-evolving agents and environments.
   - Digital Red Queen 2601.03335: historical self-play suppresses cycling.
   - Silver & Sutton 2025: the era of experience.

## 1. Evidence so far

| Line | Result | Where |
|---|---|---|
| Geb port (L2) | Reproduces Channon's Class 3 activity. Runs freeze, and are rescued by rollback. | RESULTS.md, SCALING.md |
| Complexity | Tracks capacity: plateau at 9–10 at fixed size; about 12 when world and neuron limit grow together (v8). | L3.md |
| Kin colonies, colony reproduction, division of labour (L3) | Diversity rises, bounded by capacity (D/N about 0.3). | L3.md |
| Locks (L4) | No gain over no locks (5 seeds); scale test negative so far. | L4.md |
| Signals | Listening is causal, but it carries action broadcasts. A free channel goes unused: 6 mute tests to 1M, no receivers. | SIGNALS.md, PLASTIC.md |
| Lifetime plasticity | Spreads without benefit: 7 freeze tests to 1M, no consistent effect. The arm without it has more births (2 seeds). | PLASTIC.md |
| World v1 W0 | Passes trivially. The evolved memory is a handicap in competition: the memoryless copy wins 0.70 (t +14.5). | WORLD.md §9 |
| World v1 W1 (5000 epochs) | Sustained progress fails: at best 68% and 47% of era comparisons; collapses. Historical self-play halves cycling (cyclic share 0.10–0.13 vs 0.25–0.27). Memory used by 3 of 4 final champions. | WORLD.md §9 |
| W1b / W1c / W1d | ES works (null-ES control loses 0.997). A split lineage keeping its parent's step schedule beats W1 0.94–0.998, with no collapses (2 seeds; 3 more running). One ancestor in every fork makes cycling worse. | WORLD.md §9 |
| Tournaments | 1.5M beats 250k at 0.98–1.00, still 0.98 at the old neuron limit. Neighbouring eras are within drift (2 seeds). | TOURNEY.md |
| L0 Flow-Lenia, L1 BFF | Localisation and replicators work. Selection beyond drift in L0 (6/6). | L0.md, M3.md |
| Engine | EGG plus integer EGGROLL on AMX, tested. enwik8 calibration 8.7 → 4.8 bits/byte by step 200. Paper: evolution beat backprop at equal data (3.40 vs 3.58) at about 180× compute. | EVO.md |
| Hardware | AMX int8: 2.2 TOPS per thread on raw tiles; our GEMM 0.79 TOPS per thread. About 10^20 int8 operations a year. | EVO.md |

What the evidence says:
- **Selection in these worlds produces real strategic progress** (tournaments).
- **The fixed Geb world caps it.** Its affordances are hand-designed, it holds no information worth learning or
  telling, and its only open-ended dimension was the capacity we scheduled.
- **The next system must put the open-endedness in the world.** Agents are each other's challenges, they construct
  things that persist, and the rules change with the world's state. Brains are evolved int8 learners.

## 2. Roadmap

Main line: World v1, docs/WORLD.md. Each phase has a pass criterion fixed in advance:

| Phase | Adds | Pass criterion |
|---|---|---|
| W0 | One lineage, resources, fork/evaluate/update loop | Evolved mean beats initial mean, share ≥ 0.7 over 5 seeds |
| W1 | Several lineages, splitting and extinction, historical self-play | Tournament progress above the drift band for 5k epochs; less non-transitivity with the archive |
| W2 | Private and changing information, signals (W2a); evolved plasticity (W2b) | Amnesia and deaf ablations cost fitness (t > 3); W2b also freeze |
| W3 | Programmable matter (BFF-like tapes), write action | Wiping artifacts costs fitness; distinct functional artifact types keep growing |
| W4 | State-dependent physics, observer measure | Observer compression progress stays positive; held-out-arena fitness keeps rising |

Status of older lines:
- **Geb ladder (L2–L4): frozen.** Its code stays for tournaments and comparison.
  - The L4 scale runs (runs6 l4L*) were paused on 2026-11-25 to free the CPU for World v1. They stopped at 1.96M,
    1.76M, 1.60M and 1.63M steps. They are checkpointed and can be resumed by uncommenting them in runs6/runs.txt.
  - Do not start new Geb variants.
- **L0 (Flow-Lenia) and L1 (BFF) are ingredients, not separate lines:**
  - BFF-like tapes become W3 machines.
  - L0's state-dependent rules inform W4.
  - The EGC (evolvable genetic code) results inform how machine code is encoded.
- **EGG on enwik8 is engine calibration only:** human text, so not on the critical path.
- **Transformer scaffold policy (DESIGN.md §1.1, §4): retired by rule 2.**
- **Plasticity and signal runs (runs6 pl*):** done at 1M (PLASTIC.md, verdict). Closed.

## 3. Risks and open questions

- **Compute.** About 10^20 operations a year here, against about 10^41 for biological evolution (Cotra 2020). Progress
  has to come from world design and efficient int8 evolution, not scale.
- **ES with many interacting agents is noisy.** Mirrored forks, sign shaping and historical self-play are the
  countermeasures. Raise the population before changing anything else.
- **Measuring capability without humans is unsolved.** Held-out generated arenas may stay narrow, and the observer
  measure (compression progress) is new. Cross-check the measures against each other.
- **Plateaus will come.** Rule 6 applies: diagnose what runs out, and change the world, not the bolt-ons.

---

# History: original implementation plan (M1, 2026-11)

## OEE-one implementation plan: language, structure, first milestone (Geb-class L2)

## Context
The repository has `docs/DESIGN.md` (layers L0-L5, measurement, transformer scaffold
policy) and no code. Decisions taken by the user:
- Language: C++.
- Hardware: undecided, so the design must be hardware-agnostic.
- First build: a Geb-style L2, validated against Channon's published results.

Why Geb first: it is the only system with a formal Class 3 classification, so it is the
one reference against which the instrumentation can be validated. Reproducing Geb's
result validates the agent layer and the measurement layer together.

## Language and toolchain
- **C++20**, using C++23 features only where GCC 13+ and Clang 17+ both support them.
  CMake 3.28+ with `CMakePresets.json` (debug, release, asan, tsan).
- **Hardware abstraction:** M1 runs CPU-only (std::thread / OpenMP); Geb-class agent
  simulation is CPU-bound. All per-cell and per-agent kernels go behind a backend
  interface. Kokkos is the planned GPU path for L0 (one source compiling to CUDA, HIP,
  SYCL and OpenMP), adopted at the L0 milestone, not in M1.
- **Dependencies (minimal):** GoogleTest; zstd for snapshots. RNG: xoshiro256** /
  PCG64 implemented in-tree with explicit seeding.
- **Determinism:** same seed and config produce a bit-identical state hash. This is
  required for shadow runs, ablations and the control branch in the scaffold policy.
- **Containment, which C++ does not provide at the language level:**
  - ASan, UBSan and TSan builds in CI
  - libFuzzer targets on the genotype decoder and development process, and later on
    the L1 VM
  - the simulation process runs under a seccomp-bpf allowlist with no network access
- **Python:** used only for offline analysis and plotting in `tools/`. It never runs
  inside the simulation. The transformer scaffold process is a later milestone, run as
  a separate process over IPC.
- **Commits:** author ilumCI <ilumbackup@gmail.com>, no trailers. Work happens on
  branch `ccr-a2d38bd7-kjgb66`.

## Repository layout
```
CMakeLists.txt, CMakePresets.json
src/core/            rng, toroidal grid world, scheduler, state hashing
src/geb/             genotype, developmental decoder, neural net, agent, actions
src/instrumentation/ activity statistics, component normalisation, shadow model, classifier
src/io/              config (TOML or INI), snapshot/resume, event logs
apps/oee_run/        main executable
tests/               GoogleTest unit + determinism tests
fuzz/                libFuzzer targets
tools/               Python analysis/plotting (offline only)
tests/golden/        reference outputs from the original Geb
third_party/geb_v08b3/ original Geb v08b3 source (MIT, unmodified), reference oracle
tools/geb_reference/ build and golden-data scripts for the reference
docs/GEB_SPEC.md     Geb specification derived from the source, cross-referenced to papers
```

## Milestone M1: Geb-class L2 reproduction

### M1.0 Reference oracle and specification (revised)
Revision: the original plan assumed no public Geb source. Channon publishes Geb v08b3
(C++, MIT license) at channon.net/alastair. Status:
- Vendored unmodified in `third_party/geb_v08b3/` with its license and provenance
  (`README.oee-one.md`, archive SHA-256).
- `tools/geb_reference/build.sh` builds the original headless with g++ 13
  (`geb_resets`, `geb_no_resets`); runs are byte-reproducible for a fixed seed.
- `tools/geb_reference/make_golden.sh` produced golden data in
  `tests/golden/geb_v08b3/` (seeds 1-3, both shadow variants, 1000 steps: stdout stats,
  `rulestat`, `SHADOWrulestat`, SHA-256 sums).

Consequence: M1 becomes a port verified against the oracle, not a reconstruction. The
port in `src/geb/` must match the golden outputs byte-for-byte for fixed seeds before
any modernisation (data layout, threading, RNG replacement). Every intentional
deviation after that point is gated by a flag and recorded in the spec.

Papers (for semantics and the definitions of the published tests):
- Channon & Damper (1998, 2000), Channon's PhD thesis
- the 2001 and 2002 ALife-test papers
- GPEM 2006, doi:10.1007/s10710-006-9009-3
- Artificial Life 2019, doi:10.1162/artl_a_00285, and Artificial Life 2024

Output: `docs/GEB_SPEC.md`, derived from the source and cross-referenced to the
papers, covering:
- world geometry
- genotype encoding
- the developmental system that turns a genotype into a neural network
- the sensors and actions, with how actions are bound to output neurons
- interaction rules (reproduction, killing, movement)
- the scheduling order
- the definition of a component used for activity statistics

Every item cites the source file and function, and the paper and section where one
exists. Discrepancies between the source and the papers are recorded; the source
defines the reference behaviour.

### M1.1 Core
Deterministic RNG streams; 2D toroidal grid; agent storage (structure-of-arrays);
scheduler following the spec; state hash.

### M1.2 Genotype and development
Genotype representation and mutation/crossover operators; developmental decoder into a
neural network with no fixed cap on size, other than an explicit configurable neuron
limit (needed for the 2019 joint-scaling test); neural network update step.

### M1.3 Agents and interactions
Sensing, action selection, the interaction rules, birth and death, all per the spec.
There is no explicit fitness function.

### M1.4 Instrumentation
- Bedau-Packard evolutionary activity per component; diversity; new activity; mean
  cumulative activity.
- Component-normalised activity (Channon 2002).
- A shadow model: a neutral run with the same structure and random selection, used to
  normalise and to filter out activity produced by drift alone.
- A classifier that assigns Classes 1-3 from these statistics.
- Maximum individual complexity tracking (the 2019 definition).
- Streamed logs written to disk.

### M1.5 Long-run infrastructure
Snapshot and resume (zstd), config hashing, run manifests, so runs can last months and
be resumed bit-exactly.

### M1.6 Validation against Channon
1. The shadow/neutral run must not classify as Class 3. This is the negative control.
2. The Geb run should classify as Class 3 under the component-normalised test, within
   available compute.
3. 2019 scaling, at reduced scale:
   - maximum individual complexity plateaus when world length alone is scaled
   - it plateaus when the neuron limit alone is scaled
   - it keeps increasing, roughly logarithmically, when both are scaled together

A mismatch is reported as a result. It is not tuned away.

## Later milestones (order to be confirmed after M1)
- M2: MODES metrics and multi-scale path divergence added to the instrumentation.
- M3: L1 chemistry — a BFF-style program VM (fuzzed and sandboxed) and a Hash Chemistry
  evaluator.
- M4: L0 physics — a Flow-Lenia-style substrate with state-dependent rules, on Kokkos.
- M5: L3 fraternal transitions (DISHTINY-style).
- M6: transformer scaffold process (separate process, IPC), with provenance tagging and
  automated purge triggers per DESIGN.md section 4.
- M7: L4 co-evolved challenges; L5 self-modification.

## Candidate directions (2026-11-16, after the v9 round; ranked by expected value for OEE / general intelligence)
1. Evolvable genetic code (L1 + L5): each genome carries its own codon -> operation table on a fixed
   minimal universal VM; then a heritable interpreter written in the creatures' own language; then
   heritable variation operators. Revisits M5 (faithful BFF copying gave no variation or growth).
   Test each step against a fixed-code control and a null, as in v9. Code runs only inside the VM (no
   syscalls, no network, capped resources). Refs: arXiv 2603.08463 (evolvable interpreters as an open
   question), 1105.1534 / 1108.1589 (evolvable languages, codon tables), Ofria et al. 2002 via 1309.0719.
2. Other-agent modelling as an L4 environment: agents evolve to predict what other market participants
   (or other agents) do next, on recorded data, scored against a null. A measurable route from complexity
   to theory of mind. No money at risk; trading only after held-out and paper stages with human-held keys.
3. Open-ended fuzzer (application): an ecology of input lineages with unexplored code regions as the
   resource; target the plateau that coverage-guided fuzzers hit; first target our own genotype decoder
   (also fills the missing fuzz targets of the containment plan). Refs: arXiv 2509.19349, 2505.22954,
   2601.03335, 2604.02134.
Not pursued: token issuance aimed at trend-chasers (harms late buyers; selection drifts to pump-and-dump);
real-world resource acquisition without human control of spending.

## Verification (M1)
- `cmake --preset release && ctest --preset release` passes. The same passes under the
  asan and tsan presets.
- Determinism test: two runs with the same seed produce identical state hashes at
  steps 1e3, 1e5 and 1e6; a run resumed from a snapshot matches an uninterrupted one.
- Fuzz targets run for a fixed time in CI with no crashes.
- Equivalence test: the port reproduces every file in `tests/golden/geb_v08b3/`
  byte-for-byte.
- Validation checks M1.6 (1)-(3), with plots generated by `tools/`.
