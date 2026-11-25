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
   - Post-founder share (added 2026-11-25 after the first W1 data; reported next to the protocol statistic):
     - Definition: the same integrated share, counted only after the founder generation has died (ticks > 500, the
       maximum age).
     - Why: founders start with 200 energy each, so before tick 500 the integrated share mostly measures how much
       starting energy a genome keeps, not who wins.
     - Example: in nohist s1, the epoch-500 champion against the epoch-250 one has share 0.33 at tick 500, 0.86 at
       tick 750 and 0.94 at tick 2000. Integrated share over all ticks: 0.477 (t −2.3). Post-founder share: 0.778
       (t +10.3).
     - Tools: `-curve 250` prints the share over time; `mean_lshareA` is printed by the tournament, w1_eval.sh and
       ciao.py.
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
| W2 | Seasonal, patchy, hidden resources; signals (W2a); evolved plasticity (W2b) | Amnesia and deaf ablations each cost fitness (t > 3); in W2b also freeze | Change the information structure (what is private, how fast it changes); do not add more mechanisms |
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

### W2a protocol (fixed 2026-11-25, before the code)

Literature that sets the parameters:
- **Plasticity needs change.** Its benefit is proportional to the rate of environmental change and is zero in a
  static world. Social information only lowers its cost (Le 2026, arXiv:2611.10986). The same holds in our data:
  PLASTIC.md, and the W0 amnesia test below.
- **Change faster than a lifetime.** Learning changes the course of evolution only when the environment cycles
  faster than an individual's lifetime, at a rate matched to the speed of learning (Bennati et al.,
  arXiv:1710.00352).
- **Conventions need asymmetry and two-way agents.** They emerged when every agent both speaks and listens, and the
  speaker knows what the listener does not (Toquebiau et al., arXiv:2611.00006).

World changes (`-season T -view 1 -signals 1`; all off by default, so W0/W1 are unchanged):
- **Seasons.**
  - Each of the 6 patches is productive for half of a cycle (a triangle wave in integer arithmetic), with its own
    phase. Some patch is always in season, and where that is keeps moving.
  - The period is drawn from [T/2, 3T/2) with T = 300 ticks, below the 500-tick maximum age.
  - Period and phases are redrawn every 5 epochs (1000 ticks), so the timing changes between generations and
    cannot be inherited.
  - Out of season, resources above the current capacity rot by 1 per tick, so a patch really disappears.
- **Hidden resources.** Resources are visible only within radius 1 (3×3). Agents and signals stay visible in the 5×5
  window, so whoever stands on food knows something its neighbours do not.
- **Signals.**
  - Every tick each agent also emits one of 4 symbols (0 is silence), sampled from logits 8–11 in parallel with its
    action. There is no cost (cheap talk).
  - The observation adds the symbol of each agent in the 5×5 window and the agent's own last symbol (Din = 128).
  - Signals are heard across lineages (eavesdropping is possible). Fitness is lineage biomass, so interests are
    shared within a lineage.

Measurement:
- **Ablations.** Mirrored tournaments of a champion against its intact copy:
  - `-ablate amnesia`: the recurrent state is zeroed before every step;
  - `-ablate deaf`: heard symbols are zeroed.
  - A mechanism counts as used when the ablated copy's share is below 0.5 with t < −3.
  - The same ablations in arenas with seasons off are the control: they test whether change is what makes memory
    pay.
- **Era progress** as in W1.

Calibration of resource production (2026-11-25, before the runs). Seed 1, W1 settings plus seasons, signals and
the view below:

| Regrowth | View | Rot | Result |
|---|---|---|---|
| 2 (W1) | 1 | 1 | Collapse: repeated reseeds within 40 epochs |
| 8 | 1 | 1 | Not viable: 10 reseeds in 180 epochs, about 18 agents, one lineage |
| 8 | 2 | 1 | Not viable: about 23 agents, reseeds, one split |
| 16 | 1 | 1 | Viable: about 57 agents, frequent splits, no reseeds |
| 12 | 1 | 0 | Viable: about 71 agents |

- Seasons give a quarter of the peak production on average. Out of season, rot removes the stock.
- Chosen: regrowth 16 with rot kept, so that patches really disappear. Mean production is then about twice W1's.

Runs:
- 5 seeds, 3000 epochs, W1 settings plus `-season 300 -view 1 -signals 1 -regrow 16`.
- Amended 2026-11-25, before any W2a run: plus `-split_age 1` (W1b result). The era-progress reference is W1b.
  `-hist_frac` stays 4 (W1d failed).
- Baseline arm: the same with `-signals 0` (no channel), for the deaf comparison and for the era-progress
  comparison.

Pass, kill and plasticity:
- **Pass.** In at least 3 of 5 seeds, the final champion loses under amnesia and under deaf (t < −3). Era progress is
  no worse than W1.
- **Kill (epoch 1500).** If neither ablation costs anything in any seed, change the information structure: shorter
  period, smaller view, slower redraw. Do not add mechanisms.
- **W2b (plasticity).** Start it only if amnesia shows that memory pays. Then test whether fast weights add to the
  GRU state, with a freeze ablation.

### W1b protocol: split lineages inherit the parent's step schedule (fixed 2026-11-25, before the runs)

Observation in W1 (seeds 1–2, epochs 0–1000):
- A split starts the new lineage at age 0, so α = 1/(0.015·age + 1) restarts at 1. Splits happen every ~20 epochs.
- The first active lineage had α > 0.5 in 56% (hist s1) and 92% (nohist s2) of epochs.
- Between consecutive champions 250 epochs apart, 96–97% of the weights differ, with mean |Δw| 8–11 (weight sd 16).
  This is close to an unbiased random walk of one step per epoch.
- Selection works mainly through which lineages survive and split, and little through the ES gradient.

Change: `-split_age 1`, a new lineage takes its parent's age (and so its α). This is not a new mechanism: it fixes
the reset of the step schedule. W1 itself is unchanged.

Test:
- hist arm, seeds 1–2, 2000 epochs, otherwise the W1 settings.
- **Primary.** Champions of W1b against W1 champions of the same seed at the same epoch (500, 1000, 1500, 2000),
  mirrored, 16 seeds. W1b is better if the post-founder share is > 0.5 with t > 3 at 2000 in both seeds.
- **Secondary.** Era progress, and genome change per era.
- **Decision.** If W1b is better, later phases inherit the age (W2a keeps the W1 rule, so it stays comparable to
  W1). If it is not, keep the W1 rule and record that the drift is harmless.

### W1c protocol: null-ES control (fixed 2026-11-25, before the runs)

Question: does the ES update add anything to W1, beyond lineage-level selection (who splits and who goes extinct)?

Change: `-null_es 1`.
- Each lineage's update uses random pair signs F_p (counter RNG) instead of signs from the fork fitness.
- The update size, α schedule, splits, extinction, ancestors and the world are unchanged.
- So lineages still drift by the same amount and still live or die in the canonical world, but their weights are
  not moved by fitness.

Test:
- hist arm, seeds 1–2, 2000 epochs, otherwise the W1 settings.
- **Primary.** W1 champions against W1c champions of the same seed at epochs 500, 1000, 1500 and 2000 (mirrored,
  16 seeds, post-founder share).
- **ES counts as working** if W1 wins with t > 3 at 2000 in both seeds.
- **If not**, progress in W1 comes from lineage-level selection. Next, lower the drift (W1b) or raise P, before any
  further phase relies on the ES update.

### W1d protocol: an opponent in every fork (fixed 2026-11-25, before the runs)

Diagnosis from the W1 kill check (§9):
- Competitive skill drifts whenever the canonical world holds a single lineage: 71–79% of epochs in seed 1, 12–40%
  in seed 2.
- In those epochs fitness is only the lineage's own biomass, as in W0. W0 showed that this selection does not keep
  competitive skill (epoch 500 against epoch 400: 0.31).
- An ancestor in one fork pair of four did not prevent the collapses.
- So what the world runs out of is opponents, not resources or information.

Change: `-hist_frac 1`. Every fork pair contains founders of one archived ancestor, drawn each epoch from the last
32 champions (historical self-play against the archive, as in Digital Red Queen, arXiv:2601.03335). This changes
the amount of an existing mechanism; nothing new is added.

Test:
- Seeds 1–2, 2000 epochs, otherwise the W1 settings.
- **Primary.**
  - Era comparisons from 500 to 2000: no collapse (no post-founder share below 0.25).
  - At least 3 of 4 comparisons favour the later champion with t > 3, in both seeds.
- **Secondary.** W1d champions against W1 hist champions of the same seed at 1000, 1500 and 2000.
- **If it fails**, test capacity next (a larger world, where lineages can coexist), as the Geb joint-growth result
  suggests.

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

### W0 amnesia test (2026-11-25)

- The epoch-500 W0 champion, memoryless (recurrent state zeroed every step), against its intact copy: 0.698 ± 0.014,
  t +14.5, 16/16 seeds.
- In the static W0 world, the memory it evolved is a handicap in competition. This agrees with PLASTIC.md: with
  nothing to know, keeping information does not pay. It sets the W2 change rates.

### W1 interim: memory (2026-11-25, epoch-750 champions, 8 seeds each)

Two tests, each a champion against its intact copy:
- **amnesia:** the recurrent state is zeroed every step;
- **shuffle:** agents get each other's states every step. State values stay realistic, but the agent loses its own
  history.

Post-founder share of the ablated copy:

| Run | Amnesia | Shuffle |
|---|---|---|
| hist s1 | 0.091 (t −59) | 0.165 (t −17) |
| nohist s1 | 0.223 (t −16) | 0.338 (t −8) |
| hist s2 | 0.002 (t −332) | 0.100 (t −21) |
| nohist s2 | 0.111 (t −34) | 0.121 (t −31) |

- In W1, all four champions act on their own history.
- In W0 (one lineage, no competitors), the same kind of memory was a handicap: amnesic copy 0.70.
- So competition between lineages is what made memory pay. This is the first mechanism we have seen used without
  being forced. It supports the W2 premise: information that changes, here the other agents, selects for keeping
  it.

### W1 final verdict (2026-11-25; 5000 epochs, 2 seeds × 2 arms, 16 mirrored seeds; docs/results/w1/)

Era progress, e against e−250 (19 comparisons per run), post-founder share:

| Run | t > 3 | t < −3 | Mean share | Rank correlation of epoch and rating |
|---|---|---|---|---|
| hist s1 | 13 | 5 | 0.606 | +0.70 |
| hist s2 | 9 | 7 | 0.512 | +0.04 |
| nohist s1 | 9 | 5 | 0.526 | +0.24 |
| nohist s2 | 9 | 9 | 0.519 | +0.41 |

- **Sustained progress: fail.** The criterion was at least 3/4 of comparisons with t > 3 in both seeds of one arm.
  The best arm reached 68% and 47%.
- **Non-transitivity: pass.** Cyclic share of the 21×21 champion matrix:

  | Seed | Ancestors, post-founder | No ancestors, post-founder | Ancestors, all ticks | No ancestors, all ticks |
  |---|---|---|---|---|
  | 1 | 0.102 | 0.253 | 0.057 | 0.208 |
  | 2 | 0.127 | 0.272 | 0.111 | 0.174 |

  Historical self-play cuts cycling in both seeds, as in Digital Red Queen.
- **Memory of the final champions** (share of the ablated copy, post-founder):

  | Run | Amnesia | Shuffle |
  |---|---|---|
  | hist s1 | 0.047 | 0.062 |
  | hist s2 | 0.041 | 0.119 |
  | nohist s2 | 0.042 | 0.057 |
  | nohist s1 | 0.95 (zeroing helps) | exactly 0.50 (every agent carries the same state) |

  So 3 of 4 champions use memory; nohist s1's has lost individual memory.
- **Held-out arena** (48×48, 4 patches, regrowth 1; post-founder share at 2000, 3000, 4000, 5000 against 1000
  epochs earlier):

  | Run | 2000 | 3000 | 4000 | 5000 |
  |---|---|---|---|---|
  | hist s1 | 0.31 | 0.60 | 0.83 | 0.94 |
  | nohist s1 | 0.17 | 0.68 | 0.79 | 0.05 |
  | hist s2 | 0.89 | 0.49 | 0.17 | 0.69 |
  | nohist s2 | 0.16 | 0.74 | 0.36 | 0.48 |

  Only hist s1 improves steadily in the held-out arena.
- **Diagnosis** (W1b, W1c, W1d below):
  - The ES update works.
  - The main defect is the step-schedule reset at every split. W1b removes the collapses up to 2000 epochs.
  - More ancestor exposure with one ancestor per epoch increases cycling.
- **Next.**
  - W1b-long: hist arm, seeds 1–2, extended by resume from 2000 to 5000 epochs.
  - Pass if at least 3/4 of comparisons (post-founder) have t > 3 and no share falls below 0.25, in both seeds.
    Fixed before the extension.

### W1d result: fail (2026-11-25; 2000 epochs, 16 mirrored seeds; docs/results/w1/w1d_results.txt)

- **Era progress, post-founder.** The pass condition was "no collapse, at least 3 of 4 comparisons with t > 3, both
  seeds".

  | | Seed 1 | Seed 2 |
  |---|---|---|
  | Collapses below 0.25 | 4: 1000 0.004, 1500 0.002, 2000 0.05 (and 750 0.30) | 4: 1000 0.06, 1250 0.04, 1750 0.21, 2000 0.29 |
  | Comparisons with t > 3 | 3/7 | 3/7 |
  | Mean share | 0.455 | 0.455 |

- **Cycling.** In seed 1 the shares alternate 0.9996 / 0.002 / 0.995 / 0.05 from 1250 to 2000.
- **Against W1 hist** (W1d share, post-founder): 0.75, 0.98 and 0.99 at 1000, 1500 and 2000 in seed 1. In seed 2:
  0.25, 0.38 and 0.06 (t −102).
- **Reading.**
  - One ancestor per epoch, placed in every fork, makes each update chase that one opponent. With the W1 step
    schedule this amplifies cycling.
  - Opponent scarcity was not the main cause of the W1 collapses. The step-schedule reset was (W1b).
- **Decision.** Keep `-hist_frac 4`. A variant that samples a different ancestor for each pair (a fixed mixture over
  the archive) may behave differently; it is not tested here.

### W1b and W1c results (2026-11-25; 2000 epochs, 16 mirrored seeds; docs/results/w1/w1b_w1c_cross.txt)

**W1c null-ES control** (W1 champions against W1c champions of the same seed and epoch; share of W1, post-founder):

| Epoch | Seed 1 | Seed 2 |
|---|---|---|
| 500 | 0.837 (t +16) | 0.944 (t +70) |
| 1000 | 0.993 (t +487) | 0.979 (t +144) |
| 1500 | 0.999 (t +2200) | 0.860 (t +10) |
| 2000 | 0.997 (t +777), 16/16 | 0.998 (t +928), 16/16 |

- Pass: the ES update works. W1's progress comes from the fitness-driven update, not from lineage-level selection
  alone.
- Without it there is little progress between eras:
  - post-founder t > 3 in 1/7 (s1) and 4/7 (s2) comparisons;
  - t < −3 in 2 each;
  - mean post-founder share 0.42 and 0.54.

**W1b, split lineages keep the parent's age** (W1b champions against W1 hist champions; share of W1b,
post-founder):

| Epoch | Seed 1 | Seed 2 |
|---|---|---|
| 500 | 0.804 (t +11) | 0.896 (t +16) |
| 1000 | 0.941 (t +56) | 0.905 (t +41) |
| 1500 | 0.997 (t +621) | 0.763 (t +17) |
| 2000 | 0.998 (t +722), 16/16 | 0.941 (t +52), 16/16 |

- Pass: W1b is better at every checkpoint in both seeds.
- Era progress, post-founder:

  | Run | Comparisons with t > 3 | With t < −3 | Mean share |
  |---|---|---|---|
  | W1b s1 | 6/7 | 0 | 0.84 |
  | W1b s2 | 4/7 | 0 | 0.71 |
  | W1 hist s1, same seed | 4/7 | 2 | 0.57 |
  | W1 hist s2, same seed | 4/5 | 1 | 0.66 |

  So the collapses disappear in both W1b seeds.
- **Reading.**
  - The restart of the step schedule at every split was the main defect.
  - Each new lineage spent its first ~100 epochs moving most of its weights at random. This erased what the parent
    had learned and produced the collapses, beside the monoculture effect.
- **Decision (per the W1b protocol).** Later phases use `-split_age 1`.
  - W2a has not started yet. It is amended before its runs to use `-split_age 1`, since running it on the rule now
    known to be worse would waste the compute.
  - Its era-progress reference becomes W1b instead of W1.

### W1 kill check at epoch 1500 (2026-11-25; 16 seeds, mirrored; docs/results/w1/*_interim.tsv)

The check ran after the runs had passed 1500, so it covers comparisons up to epoch 1575–2308.
- **Not killed.**
  - Many comparisons favour the later champion with t > 3 in every run.
  - One lineage holds the world in 71%, 79%, 12% and 40% of epochs (hist s1, nohist s1, hist s2, nohist s2), all
    below the 90% limit.
- **Early progress everywhere.**
  - Every comparison up to epoch 750 favours the later champion, post-founder 0.70–0.93, t +5 to +59.
  - Up to 1000 this holds in seed 1, both arms (0.93 and 0.85).
- **Then collapses.** Per run, 1–2 of 5–7 comparisons have the later champion far worse (post-founder < 0.25):

  | Run | Collapses (post-founder share) |
  |---|---|
  | hist s1 | 1500: 0.11, 1750: 0.21 |
  | nohist s1 | 1250: 0.04, 2000: 0.12 |
  | hist s2 | 1000: 0.23 |
  | nohist s2 | 1000: 0.004, 1750: 0.001 |

  About 4 comparisons per run still improve (> 0.6).
- **Not caused by young, high-α champions.** In nohist s1, lineage 39 alone held the world from 1250 to 2000, with
  α falling from 0.30 to 0.07. Against earlier champions it scored 0.04, 0.38, 0.73 and 0.12.
- **Reading.**
  - With one lineage in the world, selection is the W0 objective, so competitive skill drifts.
  - W1 runs out of opponents: competitive exclusion in a 32×32 world with about 35 agents.
  - The next test is W1d (above).
- **Priority change.** W1 seeds 3–5 are postponed behind W1b, W1c and W1d, which test the cause. The design they
  would replicate already shows the same pattern in 4 of 4 runs.

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
