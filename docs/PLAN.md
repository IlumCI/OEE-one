# OEE-one implementation plan: language, structure, first milestone (Geb-class L2)

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

## Verification (M1)
- `cmake --preset release && ctest --preset release` passes. The same passes under the
  asan and tsan presets.
- Determinism test: two runs with the same seed produce identical state hashes at
  steps 1e3, 1e5 and 1e6; a run resumed from a snapshot matches an uninterrupted one.
- Fuzz targets run for a fixed time in CI with no crashes.
- Equivalence test: the port reproduces every file in `tests/golden/geb_v08b3/`
  byte-for-byte.
- Validation checks M1.6 (1)-(3), with plots generated by `tools/`.
