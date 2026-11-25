# World v1 (W): design

Status: design, 2026-11-25. Supersedes the Geb ladder (L2–L4) as the main line. Read docs/PLAN.md §0 first; its
rules are binding here.

## 1. Goal

A world where the capability of evolved agents keeps growing without human data:
- agents learn only from their own experience in the world;
- their brains are models we evolve ourselves;
- progress is measured without human benchmarks.

The open-ended part is the world itself: agents make the challenges for each other, and they can build things that
change what others face.

## 2. Lessons that fix the design (our data)

| Lesson | Evidence | Design consequence |
|---|---|---|
| A fixed world caps progress. | Geb complexity tracks capacity (L3.md, v7/v8). In the tournaments (TOURNEY.md), the 1.5M population beats the 250k one 98–100%. Neighbouring eras are not distinguishable from drift with the current number of seeds. | Affordances must be able to grow. Agents can construct; the rules can change with the world's state (W3, W4). |
| Free signals and lifetime learning stay neutral when there is nothing to know. | 6 mute tests, 7 freeze tests and the arm comparison to 1M steps (PLASTIC.md); signalling is an action broadcast (SIGNALS.md). | Add private and changing information: patchy, seasonal, hidden resources (W2). |
| Strategy improves beyond capacity. | At the 250k-era brain limit (84 neurons), the 1.5M genomes still win 98%. | Selection works. Keep capacity evolvable (joint growth: L3 v8). |
| Integer evolution works on this CPU. | EVO.md: EGG plus integer EGGROLL implemented and tested; AMX int8. The enwik8 calibration went from 8.7 to 4.8 bits/byte by step 200. | Brains are EGG-style int8 recurrent nets trained by integer EGGROLL. Start small (1–2 layers, width 64). |
| Self-play against current opponents cycles; historical opponents suppress cycling. | Digital Red Queen (arXiv:2601.03335) in Core War. | Every evaluation includes archived ancestors (historical self-play). |
| Agent and environment pairs with transfer keep a search open-ended. | POET (arXiv:1901.01753) optimises agents with ES. | Lineages keep niches; transfer and minimal-criterion ideas apply when lineages split. |
| Niche construction can drive open-endedness. | Chiba et al. 2020 (physically grounded niche construction, ecological inheritance). | Constructed artifacts persist in the world and are inherited (W3). |

## 3. Structure

**Lineages.**
- A lineage k has a mean genome θ_k: an int8 EGG-style recurrent brain with an input projection (observation to
  width D) and an action head.
- It also has heritable settings: sigma shift and, from W2, a plasticity rule.
- θ_k is updated by integer EGGROLL from its members' episode fitness (antithetic pairs, sign shaping, thresholded
  one-bin updates; EVO.md).

**Canonical world.**
- One persistent world runs the unperturbed lineage means. Its state persists across epochs: agents, resources,
  constructed artifacts and lineage sizes.
- Births, deaths and lineage sizes happen here, with endogenous ecology:
  - An agent spends energy to reproduce; the offspring belongs to its lineage.
  - Agents die at energy 0, or from age or attack.
  - A lineage with no agents left goes extinct and is archived.
  - A lineage above a size threshold splits: the new lineage starts from θ_k plus a large perturbation and is
    optimised separately from then on.
  - At most 16 lineages.

**Epoch.** Each epoch has three parts:
1. **Fork.**
   - Copy the canonical world into 2P forks, as P mirrored pairs. Both forks of a pair have the same start state and
     the same world random numbers.
   - Each agent in a fork uses a perturbation of its lineage mean, with its own noise index; fork 2p uses +noise and
     fork 2p+1 uses −noise.
   - A fraction of the forks also contain frozen ancestors from the archive (historical self-play).
2. **Evaluate.**
   - Run each fork for H ticks.
   - An agent's fitness is its energy change plus the energy held by its descendants born in the fork (inclusive,
     endogenous).
   - Pair fitness is the sign of (+ fork − − fork), for each noise index.
3. **Update and advance.**
   - Apply the EGGROLL update to every lineage.
   - Advance the canonical world H ticks with the new means.

**Cost** (W0/W1 sizes):
- 32×32 world, about 32 agents, H = 200, P = 128 pairs, brain of 1–2 layers at width 64 (about 0.1M MACs per agent
  step).
- That is about 1.6e11 MACs per epoch, a few seconds on this CPU, so thousands of epochs a day.
- Grow world and brain only when progress flattens (joint-growth lesson).

## 4. Physics

**Grid.** 2D torus, 32×32 at first. Each cell holds:
- terrain (open or wall);
- resource (0–255) and its local regrowth parameters;
- material (one byte);
- from W3, a 16-byte program tape;
- a mark byte (stigmergy).

**Resources.**
- Regrowth is patchy and seasonal, and the period and phase change between epochs.
- An agent cannot see distant patches; it learns where they are by exploring or from others' signals.
- This creates private, changing information (W2).

**Agent.**
- State: position, heading, energy, age, lineage, noise index, and an int8 recurrent state (reset at birth).
- Observation (int8 vector):
  - a local 5×5 window: resource, terrain, material, marks;
  - other agents, tagged as same or different lineage;
  - neighbours' signal bytes;
  - its own energy.
- Actions: move, turn left or right, eat, attack (takes energy), reproduce, write (sets the material or tape byte of
  the cell in front), emit signal (byte).
- Each action has a cost in energy.

**Programmable matter (W3).**
- A cell whose material is "machine" runs its tape for up to 8 instructions per tick.
- Instructions are BFF-like, from our L1 work: move resource in or out, test resource, write mark, copy the tape to a
  neighbour (replication), and from W4, set a local physics parameter within bounds.
- Running a machine costs resource, which keeps machine replicators in check.
- Agents program machines through the write action.

**State-dependent physics (W4).** Machines can change local regrowth and movement costs within fixed bounds, so the
world's rules change with its own state (Adams et al. 2017).

## 5. Measurement (no human benchmarks)

1. **Ancestor tournaments (primary).**
   - Means from epoch t against epoch t−Δ and against the archive, in identical fresh arenas.
   - Each arena seed is played twice with the lineage slots swapped (mirrored), which cancels any advantage of a
     slot. Two identical genomes then score exactly 0.5, which checks the mirroring.
   - Score: integrated biomass share, i.e. A's biomass summed over all ticks over both lineages' sum. The final share
     is also reported, but it is nearly all-or-nothing: in the W0 control (the same genome in both slots, no
     mirroring), single games ended at 0.0, 0.3, 0.9, 1.0 and 1.0.
   - Test: mean over seeds of each mirrored pair's integrated share against 0.5, with SE and t. Use 16 seeds
     (32 games); "A beats B" means t > 3.
   - Tool: `oee_w -tourney A.egg B.egg -seeds 16 -threads 4` plus the training world flags.
2. **Held-out generated arenas.**
   - Worlds from the same generator with seeds and parameter ranges never used in training, including harder
     variants. Tournament arenas already use seeds from 1000 up, with fertility maps never seen in training.
   - Measure zero-shot fitness of current lineages against their ancestors.
3. **Ablations.**
   - Freeze plasticity (W2), mute signals (W2), wipe artifacts (W3).
   - A mechanism counts as used only when switching it off lowers fitness (t > 3, paired seeds).
4. **Open-endedness (W4).** Novelty and learnability to an observer (Hughes et al., arXiv:2406.04268), measured as
   compression progress (Schmidhuber, arXiv:0812.4360; Herrmann and Schmidhuber 2026, arXiv:2605.14831):
   - The observer is our own int8 predictor, trained by integer EGGROLL on the world-state log, never on human data.
   - Novelty: its loss on new history before it updates.
   - Learnability: the loss drop after it updates.
   - Open-ended while both stay positive.
5. **Secondary.** Activity statistics (Bedau-Packard) and diversity of lineages and artifacts.

## 6. Phases, deliverables and kill criteria (fixed before the runs)

| Phase | Adds | Pass criterion | If it fails |
|---|---|---|---|
| W0 | One lineage, resources only, fork/evaluate/update loop | Within 500 epochs the evolved mean beats the initial mean in tournaments (share ≥ 0.7, 5 seeds, outside the drift band) | Engine or world bug: fix before going on |
| W1 | Several lineages, splitting and extinction, historical self-play | Tournament share against the epoch-(t−Δ) means stays above the drift band for 5k epochs; less non-transitivity with the archive than without (paired runs) | Diagnose what runs out (resources, information, affordances) before adding mechanisms |
| W2 | Seasonal, patchy, hidden resources; evolved plasticity; signals | Freeze and mute ablations each cost fitness (t > 3) | Change the information structure (what is private, how fast it changes); do not add more mechanisms |
| W3 | Programmable matter, write action | Wiping artifacts costs fitness (t > 3); the number of distinct functional artifact types grows | Machine costs or instruction set wrong; adjust and re-test |
| W4 | State-dependent physics, observer measure | Observer compression progress stays positive; held-out-arena fitness keeps rising | Write up, rethink |

### W1 protocol (fixed 2026-11-25, before the runs)

- **Runs.** hist (`-hist_frac 4`) and nohist (`-hist_frac 0`), paired by seed. Seeds 1–2 first, then seeds 3–5.
  Settings: `-epochs 5000 -pairs 64 -H 200 -S 64 -regrow 2 -lineages 4 -nact 7 -hist_founders 8 -archive_every 25`.
- **Era progress.** The champion at epoch e against the champion at e−250, for e = 250 … 5000 (16 seeds, mirrored).
- **Non-transitivity.**
  - Take the matrix of integrated shares among the champions every 250 epochs, A_ij = logit(p_ij).
  - Split it into a transitive part (rating differences r_i − r_j, with r the row means) and a cyclic remainder
    (combinatorial Hodge decomposition; Balduzzi et al., arXiv:1806.02643).
  - Non-transitivity = the cyclic share of ‖A‖².
- **Kill (checked at epoch 1500).** Stop and diagnose what the world runs out of, without adding mechanisms, if
  either:
  - no era comparison up to epoch 1500 favours the later champion with t > 3, in any run;
  - one lineage holds the whole canonical world in over 90% of epochs, in every run (no ecology to drive a Red
    Queen).
- **Pass (epoch 5000).** Both of:
  - The later champion wins (t > 3) in at least three quarters of the era comparisons, in both seeds of at least
    one arm.
  - The cyclic share is lower with ancestors than without, in each seed pair.

## 7. Implementation (planned files)

- `src/w/world.{hpp,cpp}`: grid, resources, machines, agents, deterministic tick (counter RNG), copyable for forks.
- `src/w/brain.{hpp,cpp}`: EGG-style int8 recurrent core with observation input and action head, built on
  `src/evo` (AMX GEMM, noise, integer update).
- `apps/oee_w/main.cpp`: epoch loop (fork, evaluate, update, advance), lineage dynamics, archive, checkpoints, logs.
- `tools/w/`: tournaments, held-out arenas, ablations, observer measure.
- `tests/w/`: determinism (bit-identical across thread counts and after resume), mirrored-pair symmetry, energy
  accounting, machine semantics.

## 8. Risks

- **Compute.** About 10^20 int8 operations a year here, against about 10^41 for biological evolution. Success
  depends on world design, not scale.
- **ES with many interacting agents is noisy.** Mirrored forks with common random numbers and pair sign shaping
  reduce the noise. If updates stall, raise P before changing anything else.
- **Generated held-out arenas may still be narrow.** The generator must widen with the world (W3, W4).

## 9. Results

### W0 (runs8/w0_s1, 500 epochs, 2026-11-25)

- Settings: `-epochs 500 -pairs 64 -H 200 -S 128 -regrow 2 -threads 4 -seed 1`. About 7.5 s per epoch before the
  dead-slot skip.
- **Fork fitness.** Rose to about 6,000–8,000 by epoch 25–50, then stayed there. The changed-parameter count fell
  from 104k to 13k per epoch as α decayed.
- **Pass criterion met, but trivially.**
  - Every archived mean beats the epoch-0 mean in all 5 seeds, at share 1.0.
  - The epoch-0 mean cannot keep a population alive on its own, so this tests nothing beyond "it learned to
    forage".
- **Era progress (e vs e−100, final share, 5 seeds, unmirrored).**
  - The values ranged from 0.00 to 1.00, with a mean of 0.49.
  - The drift control (epoch 500 against itself) gave 0.30, 1.0, 0.0, 1.0 and 0.94.
  - With that measure, nothing between eras can be told apart.
- **Measurement fix.**
  - Mirrored seeds, integrated share and a t-test (§5).
  - With it, epoch 500 against epoch 400 gives 0.306 ± 0.011 (t −17, 0/16 seeds): the later W0 mean is clearly worse
    in competition.
  - This is expected. W0 selects for biomass with no competitor present, so after the foraging plateau the traits
    that matter in competition drift.
  - Competitive ability is what W1 selects for.

### W1, first attempt (2026-11-25)

- Both runs (hist, nohist) crashed at epoch 12.
- Cause: the lineage in slot 0 went extinct and no split refilled the slot, but the fork setup still read slot 0's
  freed genome.
- Fixed in apps/oee_w. ctest `w_resume` now covers this case: the old binary segfaults on its settings at epoch 23.
- Also added in the same change:
  - **Dead-slot skip.** Brains step only for living agents. Results are bit-identical: the W0 log is reproduced
    exactly and W1 trajectories are unchanged. W0 epochs went from 7.5 s to 2.2 s.
  - **Checkpoint and resume** (`-ck`). Resume is bit-exact with any thread count.
  - **Mirrored tournament** with integrated share.
