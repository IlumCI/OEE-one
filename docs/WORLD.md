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

**Programmable matter (W3; design fixed 2026-11-25 after the re-plan, PLAN.md §2).**

Literature and our own data that fix the choices:
- Self-replicators arise spontaneously in soups of BFF programs that interact, with no fitness function
  (Agüera y Arcas et al., arXiv:2406.19108). We reproduced this in L1/M3 (docs/M3.md).
- Whether replication is possible depends on the structure of the substrate, not on how rich its dynamics look
  (self-replication phase diagram, arXiv:2603.25239). So we keep a substrate where replication is known to occur:
  BFF-like tapes with two heads.
- Niche construction with ecological inheritance drives open-endedness (Chiba et al. 2020). So artifacts persist
  and are inherited by whoever finds them.

Cells:
- Each cell has a material: empty, machine or wall.
- A machine cell holds a 16-byte tape, an instruction pointer, two heads, a direction, a resource store and its
  creator's lineage. The lineage is analysis-only and is used by the wipe ablation.

Machines:
- A machine runs up to 8 instructions per tick, and pays 1 resource from its store for every 8.
- With an empty store it halts. After 200 halted ticks it decays to empty, so dead constructions do not pile up.
- Instructions are one byte each; unknown bytes do nothing:
  - BFF core: `< > { } + - . , [ ]`.
  - World operations:
    - `E` takes up to 8 resource from its cell into the store (harvest).
    - `D` drops up to 8 from the store onto the cell in front (deliver or farm).
    - `S` writes the resource level of the cell in front into the tape at head 0 (sense).
    - `C` copies the tape into the cell in front, if it is empty, making a new machine there (replicate; costs 16
      store).
    - `W` makes the cell in front a wall (costs 32), or opens a wall (costs 32).
    - `R` turns the machine (direction + tape value at head 1).
- Replicating, harvesting machines compete with agents for resources. That is a new kind of opponent and resource
  that nobody designed.

Agents (in addition to the W2a actions):
- **write.** Writes an instruction code into the front cell's tape at the agent's write position, then advances
  the position. The code is sampled from 16 extra logits, so V = 32. Costs 1 energy.
- **make machine / unmake.** Turns the empty front cell into a machine with a zeroed tape and a store of 0, or turns
  a machine back to empty and recovers its store. Costs 10 energy.
- **feed.** Moves up to 20 energy from the agent into the front machine's store.
- Observation additions (Din 128):
  - the material of each window cell;
  - the 16 tape bytes and the store of the machine in front.
- Walls block movement, attack and observation of resources behind them.

What makes this open-ended: the instruction set is universal (BFF is Turing complete). Machines can copy
themselves, harvest, deliver, sense and build. What an agent faces is therefore made by other agents and by
machines, not by us.

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

### W1e protocol: a different ancestor in each fork pair (fixed 2026-11-25, before the runs)

Basis:
- After W1b, the remaining collapses all happen in single-lineage epochs, where fitness has no competitor.
- W1d put one ancestor, drawn per epoch, into every pair. Updates then chased that one opponent, and cycling got
  worse.

Change: `-hist_pair 1`.
- Every fork pair contains founders of an archived ancestor drawn for that pair (both forks of the pair share it).
- One epoch's update therefore estimates fitness against a mixture over the archive: up to 32 champions among 64
  pairs. This is fictitious-play style historical self-play.
- It is a different way of sampling the existing ancestor mechanism; no new mechanism is added.

Test:
- W1b settings (`-split_age 1 -hist_frac 4` plus `-hist_pair 1`; hist_pair implies an ancestor in every pair).
- 2000 epochs. Seeds 3–4 first, where W1b collapsed, then seeds 1, 2 and 5.
- **Primary, over all 5 seeds.**
  - At most 1 collapse (post-founder < 0.25) in the 35 era comparisons from 500 to 2000 (W1b: 3).
  - Mean post-founder era share at least 0.70 (W1b's).
- **Secondary.** W1e against W1b champions at 1000, 1500 and 2000.
- **If it fails**, test capacity (a larger world) next.

### W3 protocol (fixed 2026-11-25, before the code)

Settings:
- W1b rules (`-split_age 1`), ancestors as decided by W1e, seasons and view 1 from W2a (memory pays there), signals
  on.
- Programmable matter on (`-matter 1`). Viability is calibrated first, as in W2a: machine costs are adjusted until
  populations persist. Calibration happens before the runs and is recorded.
- 5 seeds, 3000 epochs.
- Control arm: the same with `-matter 0` (no machines: agents cannot write and the world has no tapes).
  - Amended 2026-11-26, before any W3 run: the W2a signal runs (w2_sig_s1–s5) are exactly this arm. They have the
    same settings, including `-split_age 1 -hist_frac 4`, and the same seeds and epochs. So W3 runs only the matter
    arm.
  - The control brain has V = 16 instead of 32 (no write codes); the difference in network size is negligible.
  - W1e failed, so no `-hist_pair`.

Measures:
1. **Wipe ablation.**
   - A champion against its intact copy, mirrored.
   - In one slot, every machine made by that lineage is reset to empty every 100 ticks.
   - Machines pay if the wiped copy loses (post-founder share < 0.5, t < −3).
2. **Functional artifact diversity.**
   - Count distinct machine tapes (by hash) that live at least 100 ticks in the canonical world.
   - Group them by effect: net resource moved, copies made, walls built.
   - Track the number of effect classes over epochs.
3. **Generality** (`tools/w/generality.sh`; refined 2026-11-26, before any W3 run).
   - Champions at epoch e against e−500 in 64 arenas the run never saw.
   - Each arena draws its own settings (`-gen 1`): side 24–64, patches 3–10, season period 150–600, machine costs
     ×0.5–2.
   - Each arena is seeded with random machine programs (`-arena_progs 24` at side 32, scaled with area).
     - Program length: P(length > l) = 0.7^l. Instructions are uniform over the 16 codes. Shorter, simpler
       programs are therefore likelier, a crude simplicity prior over environments.
     - This follows Legg and Hutter's universal intelligence: performance across computable environments, weighted
       towards simple ones (arXiv:0712.3329, maximised by AIXI, arXiv:cs/0004001).
   - Two scores:
     - **solo:** each genome alone in the same arenas. Mean log ratio of post-founder biomass with t; also survival
       counts and the median. This is the absolute measure.
     - **head-to-head:** post-founder share.
   - Calibration run (seed 1, epochs 100 and 200): 100 vs 0 gives solo +10.4 (t 6.2), surviving 14/16 vs 1/16.
     200 vs 100 gives solo +0.56 (t 1.3) while head-to-head is 0.72 (t 4.4): better in fights, not yet better
     alone. The two scores measure different things.
4. **Swap test: adaptation to an intervention** (secondary; added 2026-11-26, before any W3 run).
   - In the solo generality arenas, at tick 1000 the arena's own programs are replaced by new random ones. This is
     an intervention on one mechanism of the world.
   - Recovery of a genome = log of the biomass integrated over the 500 ticks after the swap, divided by the 500
     ticks before.
   - The score is the later champion's recovery minus the earlier one's (mean, t), in the same arenas.
   - Basis:
     - Pearl: interventions, not passive data, reveal causal structure (arXiv:1801.04016).
     - Bengio et al.: a learner with the right causal model adapts faster after a sparse change in one mechanism
       (arXiv:1901.10912).
   - Faster recovery in later champions therefore indicates that they model the world's mechanisms rather than
     surface regularities.
   - A genome against itself scores exactly 0 (ctest w3_generality).
5. **Open-endedness.**
   - Our own int8 observer is trained online by EGGROLL on the canonical world-state log; no human data.
   - Novelty is its loss on new history before updating.
   - Learnability is the loss drop after updating.
   - Open-ended while both stay positive (Hughes et al., arXiv:2406.04268).
6. **Era progress** as in W1.

Pass (epoch 3000):
- The wipe ablation costs fitness in at least 3 of 5 seeds.
- The number of effect classes is still growing over the last 1000 epochs, in at least 3 of 5 seeds.
- Generality: the later champion's solo score is positive with t > 3 in at least 3 of 4 comparisons (pooled over
  seeds). The head-to-head score is reported beside it.

Kill (epoch 1500):
- Agents write or activate machines no more often than in the null arm (random-weight agents in the same world),
  and the wipe ablation costs nothing in any seed.
- Then change machine costs or the instruction set, as the W3 row of the phase table says. Do not add mechanisms.

W3b option: an emulated computer inside the sandbox (files, processes, no external network) as a richer digital
substrate. Its protocol will be fixed after W3.

### W3.1 protocol: a living substrate (fixed 2026-11-26, before the code)

Observation in W3 (seeds 1–2, epochs 325–500):
- Building is selected away.
  - Random agents: write 12.7%, make 9.8%, feed 7.0% of actions.
  - Champions: write 0.5–1.4%, make 0.1%, feed 0.2–0.7%.
  - Machines fall from about 10 to under 1 per epoch; walls stay at 13.
- What the world runs out of is reasons to care about machines.
  - The only machines are those agents build, and a machine pays only after a rare multi-step program has been
    found.
  - The W3 kill at 1500 is near certain. Seeds 1–2 run to 1500 for the formal check; seeds 3–5 are not started.

Change (the substrate's parameters, as in the BFF soups of arXiv:2406.19108; no new agent abilities):
- `-spawn r`: new random programs appear on empty cells at r per 1000 ticks per 1024 cells. They have the same
  simplicity prior as the test arenas, a store of 50 and no owner.
- `-mut m`: copying is imperfect. Each copied byte becomes a random instruction with probability m/65536.
- Machines therefore form their own evolving ecology (replicators that harvest compete for resources) whether or
  not agents build. Agents meet machines as competitors and as prey (unmake returns the store). The abilities to
  write, make and feed give agents ways to use them.
- Settings: W3 plus `-spawn 20 -mut 1024` (about 0.6 new programs per epoch-tick block of 32×32; per-byte copy
  error 1/64). Calibrated for viability first, as before.

Test (5 seeds, 3000 epochs; control arm W2a signal runs):
- As W3: wipe ablation, artifact classes, generality (solo, head-to-head, swap), era progress.
- Added:
  - **Machine ecology:** distinct tapes and effect classes in the canonical world over time.
  - **Agent–machine interaction:** rates of unmake (predation), write and feed against random-weight agents.
- **Kill (epoch 1500).** All of the following, in every seed:
  - champions' write, make, feed and unmake rates are all at or below random agents' rates;
  - the wipe ablation costs nothing;
  - the champion's solo score in seeded arenas is no better than in arenas without machines, relative to its
    ancestor.

### W3.x pilot protocol (fixed 2026-11-26, before the runs)

The arms differ in one change each. 3 seeds × 2000 epochs as a pilot; seeds 4–5 for any arm that passes.
- **A: substrate.** `-spawn 20 -mut 1024` (W3.1).
- **B: substrate plus ownership.** A plus `-own 1` (W3.2: machines belong to the maker's lineage, only the owner can
  recover a store, owners eat from their own machine's store).
- **C: substrate, ownership and split-role nodes.** B plus `-nodes 8 -node_pay 150 -node_per 200 -node_ripe 40`
  (W3.3: ripeness visible only from an adjacent cell; pays only two agents eating together).
- All arms: W3 settings (W1b rule, seasons 300, view 1, signals, regrowth 16, matter).
- Controls: W2a signal runs (no matter) and the paused W3 runs (matter, no substrate; stopped at 671 and 418).

Measures at 1000 and 2000 epochs:
- action rates of champions against random agents (`-stats`);
- wipe ablation (B, C);
- deaf ablation (all arms);
- joint harvests per 1000 agent-ticks (C);
- generality (solo, head-to-head, swap);
- era progress;
- the machine census (tapes, classes).

Pass and kill:
- **Building pays** in an arm if the wipe ablation costs fitness (t < −3) in at least 2 of 3 seeds at 2000.
- **Signals pay** in C if the deaf ablation costs fitness (t < −3) in the training arenas in at least 2 of 3 seeds,
  and joint harvests exceed random agents'.
- **Kill for an arm** (at 1000): building rates at or below random agents' and the wipe costs nothing, in every
  seed.
- **Kill for C's signal claim** (at 2000): deafness costs nothing in every seed. Then the next step is horizontal
  transfer (W3.4: a signal writes into the receiver's recurrent state), not more environment.
- Order after the pilot: W4 (law space: instructions that change local rules) if building pays; fusion of agents
  (major transitions) after W4.

References checked 2026-11-26 (web search, arXiv):
- **Communication from shared consequences.**
  - Foraging games (arXiv:2505.12872): communication emerges with interdependence. Agents fall back on implicit
    communication, watching a partner, when messages give no edge. So the deaf ablation alone can miss implicit
    signalling; the joint-harvest rate is also needed.
  - Electric-fish collectives (arXiv:2511.08436): signalling from shared consequences, with no reward for it.
  - Pheromone foraging in evolved swarms (arXiv:2212.08484).
- **Environment co-evolution (W4a).**
  - Minimal criterion coevolution (Brant and Stanley, GECCO 2017).
  - MCC-style filters on models and tasks (AC/DC, arXiv:2604.14969).
  - Difficulty-aligned coevolution (GenEnv, arXiv:2512.19682).
  - Only the selection scheme is usable here: those systems use pretrained models (rule 2).
- **Major transitions (fusion phase).**
  - Multi-level selection as the measurable sign of a transition (arXiv:2508.14232).
  - Neuronal flocks that reproduce as units (arXiv:2409.13254).
  - Transitions need incentive alignment and conflict suppression, or they are unstable (arXiv:2601.12837).
    Fused agents must therefore share fate: pooled energy and joint reproduction.

### Calibration fix before the W3.x pilot and wave 2 (2026-11-26)

- **Defect.**
  - Spontaneous programs often contain the wall instruction, and walls never disappear.
  - In the first pilot runs walls grew from 96 to 313 cells (30% of the world) by epoch 800. Populations shrank and
    reseeds rose from 2 to 6.
  - This was a substrate defect, not a result.
- **Fix.** `-wall_decay 4`: a wall erodes with probability 4/65536 per tick.
  - Mean wall lifetime is about 16,000 ticks (80 epochs, many generations), so structures still outlive their
    builders.
  - Equilibrium is about 45–50 walls at the observed build rate.
- The two affected pilot runs were moved to runs8/old_walls. All W3.x and wave-2 runs restart with the fix. Nothing
  else in the protocols changes.
- Building was already selected away in those runs (make 0.2–0.3% of actions against 10% for random agents), but the
  walls confound that, so it is not counted.

### Calibration fix 2: machine economy (2026-11-26, before the restarted runs)

- **Defect: no machine can gain energy, and copies are born dead (W3 to W3.x).**
  - A running machine pays 1 store per tick. A cell regrows at most capacity × regrow/4096 per tick, which is
    255 × 16/4096 ≈ 0.996 at the W2a/W3 settings. A harvester never earns its running cost. It can only drain the
    standing stock of its own cell.
  - `C` paid rep_cost and gave the copy an empty store. Every copy halted at once and decayed after 200 ticks, so
    machine lineages could not persist. The W3.1 "machine ecology" could not exist.
  - Probe (tools/w/mcal.cpp: one harvester `E` with store 50 on the most fertile cell, no agents): it is dead before
    tick 500.
  - This explains why building was selected away in W3 and in the first pilot runs: it could never pay back.
- **Literature.** In BFF tapes with an energy budget, tapes are viable only if the energy injection exceeds
  length × cost per operation (ε > Lc), and a new copy inherits energy (Jha et al., arXiv:2609.10817, 2026). This
  world broke both conditions.
- **Fix (machine costs only, which the W3 rule allows).**
  - `-m_cost 4`: a running machine pays 1 store per 4 ticks. A harvester is now viable on cells whose season-mean
    capacity exceeds 64.
  - The probe gains about 0.13 store per tick (50 to 893 by tick 6000). At m_cost 2 it still dies, near tick 1500.
  - `-m_give 16`: `C` also moves 16 store from the parent into the copy, so a copy runs for at least 64 ticks.
- **Calibration without agents.** Settings: 32 × 32, seasons 300, regrow 16, spawn 20, mut 1024, wall_decay 4,
  40,000 ticks.

  | Setting | Seed(s) | Machines | Distinct tapes | Notes |
  |---|---|---|---|---|
  | m_give 0 (m_cost 4 or 8) | 1 | 9–53 | — | spawn level only; no ecology |
  | m_cost 4, m_give 16 | 1 | 307 (30% of cells) | 115 | a spontaneous replicator (`-.CE]R+`) spread |
  | m_cost 4, m_give 16 | 2–3 | 34–56 | 20–34 | copiers present |
  | m_cost 8, m_give 16 or 32 | — | 490–540 (half the world) by tick 20,000 | — | too dense for agents |

  - At m_cost 4, m_give 16 the ecology is alive in every seed. Whether a replicator takes off is contingent.
  - m_cost 8 is rejected as too dense for agents.
- **Chosen: m_cost 4, m_give 16.** It is the smallest change that makes the best cells viable and gives copies
  energy.
- **Effect on the runs.**
  - The pilot and wave-2 protocols are otherwise unchanged. All 21 runs restart (runs8, binary oee_w_v14).
  - Tournaments read the world flags from each run's cmd.txt (tools/w/wflags.sh), so they include the new costs.
  - The two runs on the old economy (w3a_s1, w3b_s1) moved to runs8/old_econ. They were scored with the pilot's
    kill test at their last champions (epochs 575 and 650, 16 mirrored seeds), then stopped.
    Results: docs/results/w3x/old_econ_check.txt.

    | Run (epoch) | Champion write / make / feed | Random write / make / feed | Wipe ishare (t) |
    |---|---|---|---|
    | w3a_s1 (575) | 0.6 / 0.1 / 0.2% | 12.1 / 11.0 / 7.8% | 0.50 (t 0.36) |
    | w3b_s1 (650) | 0.4 / 0.3 / 0.1% | 12.2 / 10.3 / 7.3% | 0.49 (t −0.58) |

    The kill condition holds on the old economy, as the diagnosis predicts.
  - Side observation, not counted: in w3b_s1 the deaf copy beat the intact one (ishare 0.59, t +3.66), so hearing
    symbols cost that champion fitness.

### Calibration fix 3: a trophic link (2026-11-26, before the restarted runs)

- **Defect: once machines could live, they excluded the agents.**
  - The pilot restarted with m_cost 4 and m_give 16. In seed 1, a replicator spread to 310–420 machines (about 40%
    of the cells, the fertile ones) by epoch 30, in all three arms. Agents collapsed: populations of 1–20 and 34–51
    reseeds by epoch 230, against 2–3 in 3000 epochs in W2a.
  - The machines' upkeep (0.25 per tick each, about 100 per tick in total) used up most of the world's regrowth.
    That regrowth had fed about 60 agents in W2a. Machines also hold the fertile cells and block movement.
  - Agents could reach machine energy only by unmaking, an action they rarely choose. Champions did learn it (rate
    2% to 6–9% by epoch 200), but the canonical populations kept crashing. These runs moved to runs8/nonviable.
- **Fix 1: grazing (`-graze 1`).** Eating also takes from the store of the machine in front, once the agent's own
  cell is empty:
  - any machine when there is no ownership;
  - with ownership, wild machines and the agent's own.
  - This generalises the W3.2 granary rule. Machines become food for the agents' commonest action.
  - Arm A, seed 1, 100 epochs: 23–92 agents, 2 reseeds, fork fitness 7,800–9,000 (W2a level). Grazing keeps the
    machines at 9–16, with stores near 0.
- **Fix 2 for the ownership arms: copies are wild (`-own 2`).**
  - With `-own 1` and grazing, arms B and C still collapsed. Inspection of the checkpoints (tools/w/mcal.cpp
    `-ck`) showed why:
    - B at epoch 130: 368 of 373 machines owned by two lineages, holding 164k store.
    - C at epoch 110: 335 machines owned by one lineage with 1 agent left, holding 145k store.
  - These are evolved, agent-made replicators, some of which build walls (`{REE>CE<W_____R_`). Owned copies can
    be eaten only by their owner, so a private replicator became a plague no one else could eat.
  - With `-own 2`, only the machine an agent makes is private (a granary). Its copies are wild.
  - In arms B and C the wipe ablation then removes the machines the champion owns.
  - Arms B and C with `-own 2` and grazing, seed 1, 70 epochs: 27–78 agents, 2–3 reseeds, fork fitness up to
    12,700, 10–31 machines.
- **Calibration passes on the worst seed in all three arms.** Calibration runs: runs8/cal_graze.
- **Effect on the runs.**
  - All 21 runs restart with `-m_cost 4 -m_give 16 -graze 1`. B, C and the wave-2 arms use `-own 2` instead of
    `-own 1`.
  - Binary: runs8/oee_w_v15 (commit 86be6b7).
  - The protocols are otherwise unchanged.
- **Literature (web search 2026-11-26).** Hartvigsen and Levin's evolving lattice model of plants and herbivores
  (Princeton) found that herbivores do best at an intermediate interaction strength, and that evolution settles
  there. Here the strength of a single bite is fixed. Agents still choose how often to graze, and machines can
  evolve defences such as walls.

### Calibration fix 3 failed at longer horizons; fix 4: passable machines (2026-11-26)

- **The restarted runs collapsed after the 70–150-epoch calibration window.** Settings: m_cost 4, m_give 16,
  graze 1, own 2. Runs moved to runs8/nonviable2.
  - Seed 1, arms A and C: healthy to epoch 150 (58–68 agents, 2–3 reseeds, fitness up to 13,000). Then a new wild
    replicator variant (`_R.__E_C`, `-RC{CE+C__C_}CC{`) spread to 420–500 machines holding about 170k store by
    epoch 175. 89–101 reseeds by epoch 480.
  - Seed 2, arm A: an agent-made replicator took over from epoch 25 (458 machines, 150k store), with 54 reseeds by
    epoch 197.
  - Grazing did not stop a fast replicator. Agents graze only machines in front of them, so the inside of a colony
    stays out of reach.
- **Copies born empty (m_give 0, own 1; 400 epochs) did not stop it either.** Runs: runs8/cal_g0.
  - Takeovers occurred in 3 of 4 runs: A seed 1 (30 reseeds), C seed 1 (one lineage's farm of 150 machines with 83k
    store; 32 reseeds), A seed 2 (population 78 down to 12–35). B seed 1 stayed viable.
  - Agents feed machines by accident (0.3% of actions under the stochastic policy). Each feed lets an empty copy run,
    and the colony then holds the fertile cells.
  - Agent-free, m_give 0, about 2000 epochs: spawned harvesters accumulate slowly (m_cost 4: 35 to 137 machines;
    m_cost 8: 63 to 297).
- **Diagnosis: space monopolisation by a sessile competitor that its consumers cannot reach.** In Paine's sea-star
  removal experiment, local diversity depends on how efficiently predators prevent one species from monopolising
  the major requisites (Paine 1966; revisited by Lafferty and Suchanek, Am. Nat. 2016, doi:10.1086/688045).
  Here the requisite is the fertile cells. Machines block movement, so agents can graze only a colony's edge.
- **Fix 4: `-pass 1`.** Agents can stand on machine cells; walls and nodes still block. Eating then also grazes the
  machine underfoot, with the same rights as `-graze`. With `-own 2` every copy is wild, so any agent can eat any
  colony from the inside.
- **Calibration, 400 epochs, the worst cases** (runs8/cal_pass):

  | Run | Agents | Reseeds | Machines (epoch 50 → 400) |
  |---|---|---|---|
  | A seed 1, m_give 0 | 32–75 | 1 | 67 → 13 |
  | C seed 1, m_give 0 | 37–72 | 4 | 57 → 16 |
  | A seed 2, m_give 0 | 60–105 | 1 | 85 → 32 |
  | A seed 1, m_give 16 | 30–61 | 1 | 77 → 14 |

  - Agents now keep the machines down, and machine stores stay near 0.
  - With endowed copies, the seed that collapsed at epoch 175 is viable. So the living substrate stays:
    `-m_give 16`.
- **Viability guard (fixed now, before the runs).** A run is non-viable if its reseed count rises by more than 10
  within any 200 epochs. Such a run is stopped and reported, not scored.
  - tools/w/viab.sh checks the guard; the monitor runs it every 20 minutes.
  - Normal runs have 1–4 reseeds in 400 epochs. The collapsed runs had 31–64 in 200.
- **Effect on the runs.**
  - All 21 runs restart with `-m_cost 4 -m_give 16 -graze 1 -pass 1`, and `-own 2` in B, C and wave 2.
  - Binary: runs8/oee_w_v16 (commit 94a014e).
  - The protocols are otherwise unchanged.

### Wave 2 protocol: old and disputed ideas, one per arm (fixed 2026-11-26, before the runs)

Each arm is pilot arm C (substrate, ownership, split-role nodes) plus exactly one change. 3 seeds × 2000 epochs,
the same seeds as C, so every comparison is paired with C.

| Arm | Change | Idea and source | Targets |
|---|---|---|---|
| C+kin | `-kin_sig 1`: symbols heard by own lineage only | Cheap talk is informative only with common interest; overhearing competitors create conflict (Crawford and Sobel 1982; Godfrey-Smith and Martínez 2013, PLoS Comput Biol) | Signals unused |
| C+cat | `-cat_every 250 -cat_keep 0.1`: every 250 epochs, every agent outside a random 10% window dies; structures stay | Punctuated equilibrium; extinction events select for evolvability (Lehman and Miikkulainen 2015, PLOS ONE e0132886) | Collapses, plateau, monoculture |
| C+stress | `-stress 4`: a lineage's update fraction rises with the relative drop of its fitness below its running average | Ashby's ultrastability (1952); stress-induced mutagenesis (Cairns 1988, disputed, later shown mechanistically) | Plateau |
| C+lam | `-lamarck 1`: newborns start with the parent's recurrent state | Inheritance of acquired characters (Lamarck; disproven for genes), here as a vertical channel for culture | Memory, building (knowing where the granaries are) |

Measures (at 2000, paired with C on the same seed):
- head-to-head against C's champion (post-founder share);
- generality: solo, swap and head-to-head (tools/w/generality.sh);
- era progress: collapses and mean share;
- the arm-specific test:
  - C+kin: deaf ablation, and joint harvests against C;
  - C+cat and C+stress: collapse count and mean era share;
  - C+lam: amnesia and shuffle costs, and the wipe ablation.

Pass and kill:
- An arm **passes** if it beats C head-to-head at 2000 (post-founder > 0.5, t > 3) in at least 2 of 3 seeds, or
  meets its arm-specific criterion in at least 2 of 3:
  - C+kin: deafness costs fitness, t < −3;
  - C+cat and C+stress: fewer collapses and a higher mean era share than C;
  - C+lam: higher solo generality than C, t > 3.
- An arm with neither is closed. Ideas that pass are combined only after each has passed alone.
- Amendment (2026-11-26, before any run reached 2000): C+kin and C+lam change the world itself. Their head-to-head
  is played in both training worlds and counts only if it passes in both, so neither side has a home advantage.
  C+cat and C+stress share C's world.
- Scripts: tools/w/w3_check.sh (kill test), tools/w/w3x_eval.sh (pilot measures), tools/w/wave2_eval.sh (wave 2).
  All of them read the world flags from cmd.txt.

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

### W2a final: 5 seeds, epoch 3000 (2026-11-26; docs/results/w2a/table_5seeds.tsv, tools/w/w2_table.py)

Post-founder share (t) of the ablated copy in the seasonal training arenas:

| Seed | Arm | Amnesia | Shuffle | Deaf |
|---|---|---|---|---|
| 1 | signals | 0.000 (−3744) | 0.063 (−10) | 0.469 (−0.4) |
| 1 | no signals | 0.126 (−5.2) | 0.188 (−5.0) | — |
| 2 | signals | 0.597 (+1.5) | 0.005 (−226) | 0.473 (−0.3) |
| 2 | no signals | 0.097 (−9.1) | 0.096 (−8.1) | — |
| 3 | signals | 0.123 (−5.3) | 0.096 (−6.0) | 0.500 (0.0) |
| 3 | no signals | 0.036 (−15) | 0.093 (−8.2) | — |
| 4 | signals | 0.004 (−282) | 0.002 (−750) | 0.598 (+1.1) |
| 4 | no signals | 0.318 (−2.5) | 0.002 (−372) | — |
| 5 | signals | 0.844 (+4.6) | 0.003 (−319) | 0.722 (+3.6) |
| 5 | no signals | 0.399 (−1.0) | 0.004 (−393) | — |

- **Memory: pass.**
  - Shuffling states costs fitness in all 10 runs with seasons (share 0.19 or less, t ≤ −5), and much less without
    seasons (0.39–0.49).
  - Amnesia costs in 6 of 10.
  - In seed 5 with signals the amnesic copy wins (0.84, t +4.6) while shuffling is lethal (0.003). That champion
    depends on its own state, yet a blank state does better than it does in head-to-head play.
- **Signals: fail.**
  - Deafness costs nothing in the seasonal arenas in 5 of 5 runs. In seed 5 it helps (0.72, t +3.6), as in one
    early W3 run.
  - In static arenas it costs a little in 3 of 5 runs.
- **Era progress** (post-founder share, 500 to 3000):

  | Arm | Collapses (< 0.25) | Mean era share |
  |---|---|---|
  | signals | 2 of 5 runs, one each | 0.66 |
  | no signals | 2 of 5 runs, one each | 0.59 |

- **W2a is closed.** Memory pays where the world changes; a free channel is not used. Signals and memory are measured
  again in W3.

### W2a seed 1 final, epoch 3000 (2026-11-26; docs/results/w2a/final_s1_3000.txt)

Post-founder share of the ablated copy:

| Run | Ablation | Seasons | Seasons off |
|---|---|---|---|
| signals | amnesia | 0.0002 (t −3744) | 0.422 (t −60) |
| signals | shuffle | 0.063 (t −10) | 0.450 (t −26) |
| signals | deaf | 0.469 (t −0.4) | 0.480 (t −17.5) |
| no signals | amnesia | 0.126 (t −5.2) | 0.465 (t −7.7) |
| no signals | shuffle | 0.188 (t −5.0) | 0.459 (t −19) |

- **Memory:** pass in both arms. It pays most where the world changes.
- **Signals:** a small use, visible only with seasons off (0.48, t −17.5). The seasonal arenas are too noisy to
  resolve it (SE 0.07 against 0.001). So the pre-registered deaf criterion, measured in the training arenas, is not
  met in seed 1.
- **Era progress** (post-founder, 500 to 3000):

  | Arm | Shares at 500, 750, …, 3000 |
  |---|---|
  | signals | 1.00, 0.97, 0.75, 0.94, 0.69, 0.72, 0.53, 0.44, 0.84, 0.34, 0.47 |
  | no signals | 0.97, 0.72, 0.62, 0.48, 0.78, 0.66, 0.59, 0.32, 0.71, 0.43, 0.37 |

  No collapse below 0.25 in either arm, consistent with W1b. Progress is strong up to about 1500 epochs, then
  flat.

### W1e result: fail (2026-11-26; 5 seeds × 2000 epochs; docs/results/w1/w1e_results.txt)

- **Primary.**

  | | W1e | W1b | Needed to pass |
  |---|---|---|---|
  | Collapses (post-founder < 0.25) in 35 era comparisons | 4 | 3 | at most 1 |
  | Mean post-founder era share | 0.652 | 0.699 | at least 0.70 |

  W1e's collapses: s1 at 1750 (0.01) and 2000 (0.19); s5 at 750 (0.21) and 1000 (0.00).
- **Secondary, W1e against W1b champions at epoch 2000** (post-founder share of W1e):

  | Seed | Share |
  |---|---|
  | 1 | 0.975 |
  | 2 | 0.206 |
  | 3 | 0.069 |
  | 4 | 0.967 |
  | 5 | 0.978 |

  Mixed.
- **Decision.**
  - No `-hist_pair`; W3 keeps `-hist_frac 4` with the W1b rule.
  - The protocol's next step for the remaining collapses is capacity (a larger world). It is run only if W3 shows
    collapses too; W3 is the main line.

### W2a verdict on signals (2026-11-26; seed 3 final: docs/results/w2a/final_w2_sig_s3_3000.txt)

Seed 3, signal arm, epoch 3000 (post-founder share of the ablated copy):

| Ablation | Seasons | Seasons off |
|---|---|---|
| amnesia | 0.123 (t −5.3) | 0.499 (t −0.4) |
| shuffle | 0.096 (t −6.0) | 0.489 (t −8.4) |
| deaf | 0.500 (t 0.0) | 0.500 (t +0.2) |

- **Signals: fail, decided.** Deafness costs nothing in the training arenas in seeds 1, 2 and 3. So "3 of 5 seeds"
  cannot be reached. In seed 3 deafness has no effect even with seasons off.
- **Memory: pass in every seed so far.**
  - Amnesia or shuffle costs fitness with seasons, and much less or nothing without.
  - Seed 3 is a second clean control: 0.12 with seasons against 0.50 without.
- **Consequence (W2 row and re-plan).** No further W2 variants. Signals are measured again inside W3, where
  machines create information worth sharing. Seeds 4–5 finish for the record.

### W2a seed 2 final, epoch 3000 (2026-11-26; docs/results/w2a/final_s2_3000.txt)

| Run | Ablation | Seasons | Seasons off |
|---|---|---|---|
| signals | amnesia | 0.597 (t +1.5) | 0.463 (t −21) |
| signals | shuffle | 0.005 (t −226) | 0.454 (t −19) |
| signals | deaf | 0.473 (t −0.3) | 0.495 (t −5.6) |
| no signals | amnesia | 0.097 (t −9.1) | 0.502 (t +0.8) |
| no signals | shuffle | 0.096 (t −8.1) | 0.449 (t −28) |

- **Era progress, post-founder** (shares at 500, 750, …, 3000):

  | Arm | Shares |
  |---|---|
  | signals | 0.97, 0.84, 0.59, 0.65, 0.41, 0.50, 0.44, 0.47, 0.37, 0.52, 0.23 |
  | no signals | 0.72, 0.78, 0.72, 0.47, 0.59, 0.44, 0.72, 0.53, 0.31, 0.48, 0.59 |

  One collapse: signals, epoch 3000.
- **Seeds 1–2 together:**
  - Memory: the shuffle test costs in all 4 runs with seasons. Amnesia costs in 3 of 4. The no-signal seed-2 run is
    the cleanest control: 0.097 with seasons against 0.50 without.
  - Deafness never costs in the seasonal training arenas. It costs a little in static arenas in both seeds (0.480,
    0.495).
  - So the deaf half of the pass criterion has failed in seeds 1–2. It can still pass only if seeds 3–5 all pass.
- **Reading.** Changing hidden information selects for memory, reliably. A free channel gives at most a small
  advantage. As decided in the re-plan, signals are measured again inside W3, where machines create information
  worth sharing.

### W2a kill check, seed 1, epoch 1500 (2026-11-25; 16 mirrored seeds; docs/results/w2a/ablate_s1_1500.txt)

Champion against its intact copy. Post-founder share of the ablated copy, in the training arenas (seasons) and in
arenas with seasons off (control):

| Run | Ablation | Seasons | Seasons off |
|---|---|---|---|
| signals s1 | amnesia | 0.0007 (t −1044) | 0.525 (t +3.8) |
| signals s1 | shuffle | 0.0001 (t −12700) | 0.488 (t −3.5) |
| signals s1 | deaf | 0.534 (t +0.6) | 0.512 (t +8.8) |
| no signals s1 | amnesia | 0.275 (t −2.7) | 0.484 (t −5.9) |
| no signals s1 | shuffle | 0.094 (t −8.2) | 0.426 (t −56) |

- **Memory pays because the world changes.**
  - In seasonal arenas the champion loses everything without its own history.
  - With seasons off, the same champion does as well, or better, without it.
  - This is the W2a premise confirmed against its control: changing hidden information is what selects for
    memory.
- **The signal channel is not used.** Deafness has no cost (t +0.6 in seasons).
- **Not killed:** memory ablation costs fitness. The signal question stays open until epoch 3000 and the other
  seeds.

Seed 2, epoch 1500 (docs/results/w2a/ablate_s2_1500.txt):

| Run | Ablation | Seasons | Seasons off |
|---|---|---|---|
| signals s2 | shuffle | 0.017 (t −45) | 0.469 (t −21) |
| signals s2 | amnesia | 0.408 (t −1.0) | 0.569 (t +15) |
| signals s2 | deaf | 0.420 (t −1.9) | 0.494 (t −4.0) |
| no signals s2 | shuffle | 0.094 (t −8.1) | 0.460 (t −18) |
| no signals s2 | amnesia | 0.531 (t +0.4) | 0.509 (t +7) |

- **Own history matters because of the seasons, in both seeds and both arms.** Shuffle in seasons gives 0.0001,
  0.017, 0.094 and 0.094; with seasons off, 0.43–0.49.
- **Amnesia (zeroed state) is inconsistent:**
  - seed 1, signals: 0.0007;
  - seed 2: 0.41 and 0.53.
  - A blank state seems to be a workable fallback for the seed-2 champions; someone else's history is not.
  - Shuffle is the more reliable memory test. The protocol statistic is amnesia, and both are reported.
- **First trace of signal use.** Deafness costs a little in seed 2 with seasons off (0.494, t −4.0). It is
  significant but small, and not significant with seasons on (t −1.9).

### W1b replication, seeds 3–5 (2026-11-25; docs/results/w1/w1b_replication.txt)

- **Primary test, W1b against W1 hist at epoch 2000** (post-founder share of W1b):

  | Seed | Share |
  |---|---|
  | 1 | 0.998 |
  | 2 | 0.941 |
  | 3 | 0.990 (t +222) |
  | 4 | 0.817 (t +6.5) |
  | 5 | 0.968 (t +64) |

  W1b is better in 5 of 5 seeds. At intermediate checkpoints it loses twice: seed 4 at 1500 (0.22) and seed 5 at
  500 (0.016).
- **Era progress, 500–2000** (post-founder):

  | | W1b, 5 seeds | W1 hist, 5 seeds |
  |---|---|---|
  | Mean share | 0.70 (0.84, 0.71, 0.72, 0.61, 0.62) | ≈ 0.55 |
  | Collapses below 0.25, out of 35 comparisons | 3 | 8 |

  W1b's collapses: s3 at 1500 (0.09), s4 at 1500 (0.08) and at 2000 (0.05).
- **Correction.** The earlier statement "no collapses" came from 2 seeds and does not hold with 5. W1b reduces
  collapses; it does not remove them.
- **All three W1b collapses happen with a single lineage in the canonical world.** The monoculture effect remains
  once the drift is fixed. The next test is W1e (§6).

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
