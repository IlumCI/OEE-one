# From learners to a collective: social learning (SL1) and integration (IN1)

Design (2026-11-27). Both steps are gated on LL1 (docs/LIFE.md): each must pay alone before the next is built on it.
Never require two new capabilities to appear together (Eigen's paradox: WORLD.md, agent study; the bootstrap rule).
- SL1 starts only if LL1 passes: learning is used, and freezing it costs fitness.
- IN1 can start in parallel with SL1 once LL1 passes, because roles are expected to emerge through individual
  learning.

## What the v-series taught (Geb ladder, docs/L3.md)

| Step | Mechanism | Result |
|---|---|---|
| v4 | Kin colonies with evolvable grouping | Diversity plateau 2.5–3 times every control (6 vs 6, complete separation) |
| v5 | Colony reproduction (propagules, size cap 32) | Plateau slightly higher (+25%, not significant) and more uniform; organisation saturates early |
| v6 | Division of labour by a world rule (mixed warrior/breeder groups fight at double power) | Roles appeared and sustained activity, but did not raise the plateau; one seed lost its roles (warriors 0.41 to 0.18) |
| v7–v8 | Growing world, joint growth | The plateau tracked capacity |

Lessons:
- A new level of organisation adds room once, not continually.
- Roles imposed by a rule are not stable, and they do not raise the ceiling.
- Those agents could not learn.
- So this time nothing about roles is imposed. The colony is given only means of interdependence, and roles are
  left to emerge where they pay.

World v1 lineages are already colonies in the sense that matters for a higher-level individual:
- clones sharing one genome (low conflict, relatedness 1);
- selected as a unit (fitness is lineage biomass);
- reproducing as a unit (splits).

What they lack is what turns a society into an organism (Queller and Strassmann 2009, Phil. Trans. R. Soc. B 364:3143):
- knowledge passed between members;
- interdependence;
- division of labour.

## SL1: social learning (the colony learns, not only its members)

Evidence:
- **Rendell et al. 2010** (Science 328:208), a tournament of social learning strategies in a changing environment.
  Strategies that relied heavily on copying won, even when copying cost as much as trial and error. Copied
  individuals tend to show their best behaviour, which filters information for free.
- **Accumulation in learning agents.** Agents that accumulate across generations outperform those trained for a
  single lifetime with the same experience (Cook et al., NeurIPS 2024, arXiv:2406.00392).
- **Cultural transmission without human data.** It arises and generalises in learning agents (Bhoopchand et al.,
  Nature Communications 2023).

Mechanism, two arms on top of the best LL1 arm. Both use channels that exist in animals, not copying of synapses at
a distance:
- **V (teaching at birth):**
  - A newborn's plastic synapses start at half its parent's learned synapses. Traces start at zero; the baseline is
    copied.
  - Own experience overrides what was inherited.
  - This is vertical cultural transmission. It goes through the parent, not through the genome, so it does not
    wait on evolution.
- **O (observational learning):**
  - Each tick, an agent also learns from one kin agent in its 5×5 window: the nearest, ties broken by slot.
  - It applies the three-factor rule to its own features h (what it perceives), with the neighbour's action and the
    neighbour's reward prediction error: "what a neighbour just did, and whether it paid".
  - Learning rate half of its own.
  - Animals learn from others' outcomes this way: observational conditioning and local enhancement.

Measures (as LL1, plus):
1. **Transmission is used:** the champion with transmission blocked (V: newborns start at zero; O: no observational
   updates) against an intact copy. Used if the blocked copy's share is below 0.5 with t < −3.
2. **Accumulation:**
   - the futile share of newborns (ages 0–49) against the LL1 arm;
   - in V, whether newborn futility keeps falling over the run (a cultural ratchet), not only within lives.
3. **Strength** against the LL1 champions of the same seeds.

Pass, per arm: transmission is used in at least 2 of 3 seeds, and strength against LL1 is at least 0.5.
Kill, per arm: blocking transmission costs nothing (|t| < 2) in every seed.

## IN1: integration (interdependence and division of labour)

Evidence:
- Division of labour is favoured by positional effects, accelerating returns and synergy between parts, and
  selection for robustness works against it. The conditions are restrictive (Rueffler, Hermisson and Wagner 2012,
  PNAS 109:E326).
- Specialists emerge in learner populations from competition alone (arXiv:2601.19943, 2026), and when tasks can be
  done in parallel (arXiv:2503.15703).
- In social insects, roles emerge without fixed castes when doing a task lowers the threshold for doing it again
  (response-threshold reinforcement; Theraulaz, Bonabeau and Deneubourg 1998, Proc. R. Soc. B 265:327). LL1's
  plastic layer is that reinforcement.

Mechanism: **give**, a new innate action. An agent passes up to 20 energy to the kin agent in front, keeping at
least 1.
- **Why it is needed.** Without it every agent must feed itself, so no member can specialise in anything that does
  not feed it: guarding, fighting, building or tending farms. Food sharing (trophallaxis, provisioning) is the
  interdependence that social insects and multicellular bodies rest on.
- **Why innate.** Like reproduction, it costs now and pays through kin. The plastic layer does not act on it;
  evolution decides when to give.
- **What can drive roles in W1b.** Fighting: W1b is a war world, where 70% of deaths are kills and agents attack
  rivals far more than kin. Fed fighters and feeding foragers is the soldier and worker division of ants.
- **The building worlds** (nodes, farms) add synergy and accelerating returns, the stronger conditions for division
  of labour. IN1 moves there once it passes in W1b.

Measures:
1. **Giving is used:** the champion that cannot give against an intact copy (share < 0.5, t < −3).
2. **Division of labour** (Gorelick, Bertram, Killeen and Fewell 2004, Behav. Ecol. Sociobiol. 56:242). The index
   is the mutual information between individual and action, normalised by the action entropy, over each agent's
   life (probe records).
   - It is compared with a shuffle null (actions permuted across agents within age bins) and with the same arm
     without giving.
3. **Interdependence:** removing the top tenth of agents by giving harms the colony more than removing a random
   tenth.
4. **Strength** against the champions of the previous step.

Pass: giving is used in at least 2 of 3 seeds, and the division-of-labour index is above the shuffle null and the
no-give arm in at least 2 of 3 seeds.
Kill: giving costs nothing in every seed, or the index is at the null everywhere.

## Next, if both pass: IN2, a shared colony memory

- A persistent, decaying mark that agents leave on cells, which only their kin sense (stigmergy; Grassé 1959; ant
  trails).
- It is an external memory owned by the colony, and the channel through which a superorganism's knowledge outlives
  its members.
- Kept for later to respect the bootstrap rule: memory pays only once members can learn, teach and depend on each
  other.

## Implementation (2026-11-27; off by default, existing runs unchanged)

| Flag | What it does | Ablation (tournaments, agent A) |
|---|---|---|
| `-life_teach 1` | SL1 V: a newborn's synapses start at half its parent's (Life::inherit) | `-ablate noteach` |
| `-life_obs 1` | SL1 O: after its own update, each agent learns at half rate, without a trace, from the nearest kin agent in its 5×5 window that acted this tick: that agent's action and reward prediction error, applied to its own features and policy (Life::learn_obs) | `-ablate noobs` |
| `-give 1 [-give_amt 20]` | IN1: action give (index 7 without matter, so `-nact 8`; 10 with matter, `-nact 11`) passes up to 20 energy to the kin agent in front, keeping 1. Observation entry 63 (din 64) or 127 (din 128) shows the energy/8 of the agent in front if it is kin (a cue of need). Giving is innate (not plastic); for the giver's learning it counts as kept (inclusive accounting) | `-ablate nogive` |

- **Measures:**
  - tools/w/probe.py reports the division-of-labour index (I(agent; action)/H(action)) and its shuffle null;
  - `-stats 1` reports energy given per thousand agent-ticks.
- **Tests:**
  - test_w_world: teaching halves the parent's synapses; observing a paying action raises it; give reaches kin
    only; the need cue.
  - ctest w_social: bit-exact resume for any thread count, mirror 0.5, the three ablations run.
- **Runs** (after LL1 passes, on its best arm A; W1b, seeds 1–3, 2000 epochs):
  - SL1 V: `-life 1 <arm A> -life_teach 1`.
  - SL1 O: `-life 1 <arm A> -life_obs 1`.
  - IN1: `-life 1 <arm A> -give 1 -nact 8`.


## SL1 V in the LL1c world (protocol fixed 2026-11-28 01:42 UTC, before the runs)

Chosen when the owner's decision timer expired with the recommended option applied, after the LL1c replication
(docs/LIFE.md).
- The gate is met: learning is used, and freezing it costs fitness (12 of 12 learning runs).
- The measured cause of learning not paying is the hiding effect. Each newborn relearns from zero, the genome leans
  on learning, and innate competence decays: frozen learners win 0.02–0.22 against E genomes.
- Teaching at birth attacks that cost. A newborn starts with half its parent's learned synapses (Life::inherit), so
  the lineage carries what it has learned across generations (vertical cultural transmission).

Runs:
- **SV:** the LL1c L arm plus `-life_teach 1`, seeds 1–6 (runs8/w1k_SV_s{1..6}, binary runs8/oee_w_v29).
- **Controls:** the existing LL1c runs of the same seeds, E (no learning) and L (learning without teaching).

Measures (final champions, fresh held-out arenas, 16 seeds × 2; tools/w/ll1c_eval.sh):
1. **Teaching is used:** `-ablate noteach` (A's newborns start at zero) against an intact copy.
2. **SV beats evolution alone:** against the E champion of the same seed (B does not learn).
3. **SV beats learning without teaching:** against the L champion of the same seed (B does not teach,
   `-teach_b 0`).
4. **Learning used:** freeze, as LL1c.
5. **Newborns:** the toxic share of eats at ages 0–99, against L. Teaching should lower it.
6. **Hiding:** SV frozen against E, as docs/results/ll1c/hiding.txt.

Pass:
- teaching is used (noteach t < −3) in at least 4 of 6 seeds;
- and SV beats E (t > 2) in at least 4 of 6.

Secondary:
- SV beats L (t > 2) in at least 4 of 6;
- the pooled share against E is above 0.5 (t > 2 over seeds).

Kill: noteach costs nothing (|t| < 2) in every seed, or SV beats E in at most 2 of 6. Then transmission of learned
synapses does not turn learning into a lineage advantage.

Viability: reported, not an exclusion (as LL1c).

Smoke check before the runs (2 arenas, not a result): the LL1c L seed 1 champion with teaching switched on beats its
own copy without it, 0.84.
