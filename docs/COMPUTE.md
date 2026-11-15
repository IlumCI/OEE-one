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
Recommendation: build it as a second engine behind a flag, keep the asynchronous engine as the
reference, validate, then use the synchronous engine for all large runs.

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
