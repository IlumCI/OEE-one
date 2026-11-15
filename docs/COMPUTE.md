# Compute architecture plan

Goal: carry the simulation to much larger worlds, populations and brains on limited hardware,
and fix the decisions that would otherwise force a rewrite later.

## Measurements (2026-11-15)

Profile: callgrind on the v8 seed-1 checkpoint at 3.5M steps (48x48 world, ~1,750 agents,
neuron limit 76), 60 steps, 13.7e9 instructions. Inclusive share of total:

| function | share | what it does |
|---|---|---|
| `Organism::copy_net_inputs_from_neighbourhood` | 55.3% | sensing: for each input node, scan the (2c+1)^2 neighbourhood and every neighbour's output nodes for prefix matches |
| `Organism::actions` | 29.7% | of which reproduce 16.3%, kill 9.9% |
| `World::group_size` (BFS) | 11.7% | recomputed from scratch on every birth and every attack |
| `World::reproduce` + `Organism::Organism` (development) | 9.1% + 7.5% | crossover/mutation, new network |
| `Ann::update_outputs` | 8.6% | the neural network itself |
| `operator new` / `delete` | 6.5% + 6.3% | per-event heap allocation (BFS sets, vectors) |

The neural network is not the bottleneck (8.6%). Sensing is: its cost grows with
inputs x neighbours x neighbour outputs, i.e. roughly quadratically in network size, which is
why v8 (growing neuron limit) slowed ~25-30x. Group bookkeeping and allocation add ~30%.

Storage per finished 4M run (L40 / growing worlds): 0.7-1.2 GB, of which `presence`
(text, hex component hashes per snapshot) 55-60%, checkpoint archives ~25%, rulestat ~10%.

Throughput: 20x20 world ~1.1M steps/h; 48x48 with neuron limit 76 ~40k steps/h. Each run is
single-threaded; parallelism is only across seeds.

## Decisions to fix now

### D1. Update semantics (the one decision that is expensive to change later)
Current: Geb's asynchronous random-order update (one agent at a time; each sees the effects of
all earlier agents). It is inherently sequential, so a single world cannot use more than one
core or a GPU.

Alternative: synchronous two-phase update. Phase 1, every agent senses and computes outputs
from the same frozen state (parallel). Phase 2, actions are applied with deterministic conflict
resolution (e.g. ordered by a per-cycle hash of agent id). Randomness from a counter-based
generator keyed by (seed, cycle, agent id) (Philox), so results are identical for any thread
count. This is the standard route to parallel, reproducible agent-based models (double-buffered
spatial grids; arXiv 2606.21784, 1507.04047, 2105.00039).

Cost: no longer bit-identical to Geb; equivalence must be shown statistically (Channon
classification, D and complexity curves on the reference configurations already run).

Option C (recommended): keep the asynchronous random-order semantics and parallelise it
order-equivalently. Generalised problem: execute a fixed sequence of local updates on shared state
in parallel with a result identical to serial execution. Solved in other fields:
- statistics: any fixed sequential (Gauss-Seidel) schedule has an exactly equivalent parallel
  execution from a colouring of the dependency graph (GraphLab arXiv 1006.4990; chromatic MCMC
  arXiv 1612.00595);
- deterministic databases / blockchains: optimistic parallel execution against a preset serial
  order, conflicting transactions re-executed in order (arXiv 2602.03444, 2602.21566);
- parallel discrete-event simulation: speculative execution with rollback (Time Warp; PARSIR
  arXiv 2410.00644; Warped2 arXiv 2507.18050).
Here: agents farther apart than their interaction radius commute within a cycle, so the per-cycle
random order is executed in parallel and any update whose read/write cells overlap an earlier,
unfinished one is deferred or re-executed in order. Preconditions:
1. Per-agent counter-based RNG keyed by (seed, cycle, agent id) replacing the single shared stream
   (the only semantic change; distributionally identical; validated once statistically).
2. Locality: replace the global random cell for births into a full world with a local rule
   (or treat it as a serialising event), and bound group queries via D3.
Result: bit-identical output for any thread count, Geb update semantics preserved.

### D2. Sensing index (no semantic change)
Per cycle, each agent publishes its output values into a small table keyed by output-node prefix
(prefixes are short bit strings); sensing then looks up matches instead of scanning every output
node of every neighbour. Expected: the 55% share drops several-fold, and cost becomes ~linear in
network size. Must stay bit-identical (golden logs).

### D3. Incremental group structure (no semantic change)
Maintain bond groups incrementally (union on bond; recompute only the affected component on
unbond or death) instead of a BFS with fresh hash sets on every birth and attack. Removes most of
the 11.7% and much of the allocation.

### D4. Memory (no semantic change)
Pool allocators for organisms, networks and per-event scratch; structure-of-arrays for hot agent
fields (position, direction, colony, role) to make D1's phase 1 vectorisable.

### D5. Storage and analysis
- `presence`: binary, delta-coded per snapshot, zstd-compressed (expected 5-10x smaller).
- Checkpoint archives: log-spaced retention (e.g. keep 250k, 500k, 1M, 2M, 4M and the last two).
- Analysis (activity statistics, D, complexity series) in C++ reading the binary format; the
  Python tools stay as the reference implementation and test oracle.

### D6. Distribution
Seeds and conditions are independent, so the cheapest scale-out is more machines, not bigger
ones: the supervisor gets a host list, runs are placed by free cores, checkpoints are portable
(already machine-independent). Works with spot/preemptible capacity because every run resumes
from its last checkpoint.

### D7. GPU
Agents here have per-individual network topologies grown by development; that is irregular and
maps poorly to GPUs. The GPU-native ALife systems (JaxLife arXiv 2409.00853; Microcosmos
arXiv 2607.02954, 500k particles on one GPU; Lenia at scale arXiv 2304.05639) use regular
substrates. Plan: GPU for the regular layers (BFF soups, any continuous-field physics layer);
multicore CPU with D1 for the agent layer. Revisit if the agent model moves to fixed-shape
networks.

## Order of work
1. D2, D3, D4 (no semantic change; validate bit-identical against golden logs). Immediate speedup
   for all current experiment types.
2. D5 (storage and C++ analysis).
3. D1 synchronous engine behind a flag; statistical validation against the reference runs.
4. D6 multi-host supervisor.
5. D7 for the soup layers when they come back into use.

## Step 1 results (2026-11-15)

Verified on the v8 seed-1 checkpoint at 3.5M (48x48, ~1,750 agents), 2,000 steps: log, presence,
rulestat, shadow rulestat and final checkpoint byte-identical to the previous binary; 10/10 unit
tests pass. CPU time 215.8 s -> 172.6 s (-20%). Instruction profile (60 steps): 13.7e9 -> 10.8e9.

| change | effect | kept |
|---|---|---|
| group traversal with visit stamps and reused buffers (no hash set / allocation per call) | group_size 1.61e9 -> 0.41e9 | yes |
| compact list of non-zero outputs scanned in sensing | sensing 7.6e9 -> 5.9e9 | yes |
| outputs sorted by character + binary search for the matching run | sensing 7.6e9 -> 11.7e9 (slower) | no |
| per-neighbour output terms precomputed at first match | sensing 5.9e9 -> 7.7e9 (slower) | no |

Finding: most input/output comparisons in sensing are matches (line profile: the match test and the
accumulation have similar counts), so sensing is real work proportional to the number of matches,
not wasted scanning; indexing cannot remove it. The remaining large lever is parallelism (D1
option C), not further single-thread tuning. Remaining single-thread item: per-event allocation
(operator new 5.5%).

## Step 2: counter RNG streams (2026-11-15)

Flag `-crng` (`Params::rng_mode` = 1). Three streams, each keyed by (seed, cycle, agent id) and
re-keyed at the start of the agent's update: real-agent draws (R1), first shadow population (R3),
second shadow (R2). The cycle's update order is a permutation of the cycle's agent list drawn
from a per-cycle stream; `step()` takes agents from the end of it. Legacy mode (default) routes
all three through Geb's ran1 in the original order.
Checkpoint format v9 (magic `OEE9GEBW`) stores the full 36-byte Params; older checkpoints store and
read the first 32 bytes (rng_mode = 0).

Checks: legacy mode byte-identical to the previous binary (log, presence, rulestat, checkpoint) on a
fresh 12k-step run and on the v8 3.5M checkpoint; 10/10 unit tests. Counter mode: identical output
on repeat; resume at 10k = uninterrupted 20k (log and presence identical).
Statistical validation (queued): `crng_v6_s1..s3` (v6 rules, L20, 2M) vs `l3v6_s1..s3`;
`crng_geb_s1..s2` (plain Geb, L20, 1M) vs `runs/resets_s1..s5`. Compare D (rule) and maximum
individual complexity per 500k window.

Result (v6 rules, L20; `docs/results/compute_crng_validation.md`): mean over the 1-2M windows,
crng seeds 1-3 vs legacy `l3v6_s1..s3`:
- maximum individual complexity 6.70 / 7.72 / 7.34 vs 7.71 / 7.18 / 7.53 (exact two-sided
  permutation p = 0.7);
- D (rule) 76 / 88 / 99 vs 119 / 86 / 88 (p = 0.7);
- real D 8-15x its null in both.
No detectable difference at n = 3 per condition (low power: differences of roughly one seed-to-seed
standard deviation would not be detected).

Result (plain Geb, L20, 250k windows; mean over the 250k-1M windows), crng seeds 1-2 vs legacy
`runs/resets_s1..s5`:
- maximum individual complexity 11.27 / 11.20 vs 10.61 / 10.89 / 11.48 / 10.38 / 10.80 (exact
  two-sided rank-sum p = 0.38);
- D (rule) 20.1 / 21.1 vs 16.8 / 20.3 / 22.0 / 15.8 / 19.7 (p = 0.57);
- D null 3.9-4.5 vs 3.1-5.1.
No detectable difference on either rule set. Counter streams accepted as the default for new runs;
legacy mode stays for bit-exact Geb reproduction.

## Step 3: locality (analysis, 2026-11-15)

What an agent update reads and writes, and what is non-local:
- Sensing: cells within `copy_to_length` (3). Movement, attack, reproduction: the cell in front and
  the one beyond it (reproduction places the child in front of the mate, or replaces the mate).
  All local. The global random cell (`random_occupied_cell`) is used only by the GebR control.
- Bond groups: traversal over in-range bonds; bounded by colony size (32 under v5+ rules) but not by
  a fixed radius. Handled as a dynamic read set in step 4.
- Population counter: read against `min_num_individuals` (kills, senescence) and
  `max_num_individuals` (births). With max = world size the max check is implied by the target cell
  being occupied. Exact rule for step 4: execute in parallel only while the population is farther
  from either limit than the batch size; otherwise serial.
- Agent ids (`next_id_`), `byid_`, colony ids of propagules: assigned at commit, in serial order.
- Shadow populations: global, but they never interact with real agents; with counter streams their
  draws are independent of the real-agent draws, so shadow updates are logged per agent and applied
  at commit in serial order.
- Counters (`num_reproductions_`, `propagules_`) and statistics: commutative, or computed at cycle
  boundaries.
Conclusion: no ecological rule has to change for order-equivalent parallel execution; step 4 is an
engine change (batching, conflict detection on cell read/write sets, commit in serial order).

## Step 4: order-equivalent parallel engine (2026-11-15)

`-threads N` (counter-RNG mode only; GebR stays serial). Each agent update is split into phase a
(sense, develop, network update: reads cells within c = `copy_to_length`, writes only the agent's
own network) and phase b (actions: reads/writes cells within 2 of the actor; a newborn senses within
c of itself, so within c + 2 of the actor; group traversal, colony counts, shadows, ids and
population counters are read and written only here). At the start of a cycle every agent sits in
its own cell, so for agent t in cycle order the dependency is `dep[t]` = the latest earlier agent
whose start cell is within R = c + 2 (toroidal Chebyshev). Phase a of t runs on a worker as soon as
`dep[t]` has committed; phase b runs on the calling thread strictly in cycle order. No commit after
`dep[t]` can reach t's neighbourhood before t commits, and no commit reads a phase a that is still
running, so the result is the serial counter-RNG run exactly (generalised problem: deterministic
optimistic execution against a preset serial order, as in deterministic databases / Block-STM, with
the read/write sets known in advance from locality instead of discovered at run time).

Also `-crng` on `-resume` of a legacy checkpoint switches the run to counter streams from that
point (`World::to_crng`, a new stochastic branch, like `-reseed`).

Checks:
- byte-identical log, presence, rulestat, shadow rulestat and checkpoint, serial vs 2/3/4 threads:
  plain Geb L20 (20k steps), v6 rules L20 (20k), v8 seed-1 checkpoint at 4M (52x52, ~2,050 agents,
  neuron limit 84; 400 steps);
- ThreadSanitizer: no reports (v6 rules L32 3k steps; v8 checkpoint 10 steps, 4 threads);
- 10/10 unit tests (legacy golden equivalence unchanged).

Speed, v8 checkpoint, 400 steps (wall time incl. load): 1 thread 38.3 s, 2 threads 23.3 s (1.6x),
3 threads 18.3 s (2.1x), 4 threads 17.1 s (2.2x). On L20 worlds the dependency window (11x11 of
20x20 cells) leaves little parallelism and threads are slower (31 s -> 41 s): use 1 thread there.
The ceiling is the serial phase b (actions, group traversal, births with development of the
newborn: ~35-40% of the work), so Amdahl caps this design near 2.5-3x. Next lever: run phase b of
non-conflicting agents in parallel too (needs the global items of step 3 handled at commit).
