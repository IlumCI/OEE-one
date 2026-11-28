# Lifetime learning (LL1): agents that learn from their own experience

## Why

The agent study (WORLD.md, 2026-11-27) found that every agent so far has fixed weights from birth to death. Evolution
is the only learner, and it is a weak one:
- It sets base rates (how often to move, eat, attack) and a few coarse conditions, then stops improving.
- About a third of all actions have no effect, in every world and at every epoch.
- A copy that skips them beats the champion decisively (0.69–1.00 share in all eight champions tested).
- Single weights change as if unselected, so conditions that do form can erode.
- More fork pairs per update and input normalisation have not removed the waste so far (LT1, interim).

Earlier learning in this project was not reward-driven:
- The Geb agents had Hebbian plasticity (PLASTIC.md). Synapses changed with correlated activity, but no signal said
  whether an action turned out well. In 7 freeze tests it was neutral, and the line was closed.
- The current agents have recurrent memory. It is used (with seasons, wiping it costs fitness), but it is memory, not
  learning from outcomes.

What is missing is a third factor: a reward signal that acts on the agent's own synapses during its life.

Information rate:
- Evolution receives one comparison per fork pair per update: 64 per lineage per epoch.
- A reward signal per agent per tick gives about 10^5 per epoch in W1b.
- What an agent learns cannot be eroded by drift between generations, and it starts from zero in every life.
- Learning evolves when "today predicts tomorrow" within a lifetime (Stephens 1991, Behavioral Ecology 2:77). In
  these worlds, food underfoot, a blocked cell ahead or a rival in front each persist for a while.
- Learning can also guide evolution: it makes good behaviour pay before the genes for it are found (Baldwin effect;
  Hinton and Nowlan 1987; computationally arXiv:2505.10651).

## Mechanism

**A plastic action layer with a three-factor rule.** This is dopamine-gated plasticity at the synapses from cortex
to action selection. In machine-learning terms it is a contextual bandit learner with eligibility traces (REINFORCE,
Williams 1992). Recent work: three-factor rules (pre × post × reward) over two-factor Hebbian rules in
neuroevolution (NEOL, arXiv:2606.20817, IJCAI 2026); evolved networks with reward-gated plasticity that learn new tasks
during life (Miconi, arXiv:2112.08588); the theory of three-factor rules (Gerstner et al. 2018, Front. Neural
Circuits 12:53).

Per agent, in addition to the inherited network:
- **Synapses** W[a][j] from the network's top-layer state h (D = 64, int8) and a bias input (64) to each plastic
  action's logit. They are zero at birth, so a newborn acts exactly as evolution made it. Learned synapses are not
  inherited (Baldwinian).
- **Action logits.** Inherited logit plus (W·x) >> 16, clamped to int8. The world samples from these logits as usual.
- **Eligibility.** Every tick, e[a][j] = λ·e[a][j] + (1[a chosen] − π_a)·x_j. This is the gradient of log π(chosen
  action) for the plastic logits, π computed from the logits actually used.
- **Reward prediction error.** δ = ΔE − b, where ΔE is the change in the agent's energy over the tick and b is the
  agent's running mean of ΔE (decay 2^−β).
  - ΔE is a world quantity: food eaten, minus the costs of living, moving and acting, minus energy lost to attacks.
- **Update.** W += η·m·δ·e, with m = 1 (fixed rule) or m from the network (modulated rule, below).

Innate actions:
- Reproduction pays only later, through descendants, and costs energy now. A one-tick energy reward would suppress
  it, so by default its logit is not plastic.
- In the building worlds, write, make and feed also invest now for later and stay innate.
- Arm R tests this choice.

Integer arithmetic (as everything else): W and e are int32 in fixed point, π comes from the world's own sampling
table, and all updates are deterministic. Runs stay bit-exact and resumable, and the plastic state is saved in
checkpoints.

## LL1 protocol (fixed 2026-11-27, before the runs)

Runs: runs8/w1l_L_s1..3, w1l_LR_s1..3, w1l_LM_s1..3 (binary runs8/oee_w_v27).

World: W1b, as LT1 (H 200, 64 pairs, 4 lineages, 7 actions, historical self-play every 4th pair, split lineages keep
the parent's age), seeds 1–3, 2000 epochs. Control: runs8/w1b_hist_s1..s3.

Arms:
- **L (fixed rule):** plastic noop, forward, left, right, eat, attack; reproduction innate; m = 1.
- **LR (reproduction plastic):** as L, but reproduction is plastic. A birth's reward counts the energy given to the
  newborn as kept (inclusive: ΔE + 50), so a birth costs only its 20-energy overhead.
- **LM (modulated rule):** as L, but m = (logit_m + 128)/128 in [0, 2), from an otherwise unused output of the
  network (logit 15; 11 in matter worlds). The network itself decides when, and how strongly, to learn. Evolution thus
  shapes the learning rule, not only the behaviour (neuromodulation computed by the network; Miconi et al. 2019,
  "Backpropamine"). At m = 1 it is the fixed rule.

Learning rate η, trace λ and baseline β: chosen before the runs by calibration on the control champion (W1b seed 1,
epoch 2000), learning on against learning off (2026-11-27, 16 seeds × 2).

Calibration. Share of a frozen copy against a learning copy; below 0.5 means learning helps:

| η (2^−x) | λ | β | Frozen copy's share |
|---|---|---|---|
| 6 | 230 | 4 | 0.995 (t +662) |
| 8 | 230 | 4 | 0.93 (t +35) |
| 10 | 230 | 4 | 0.48 (t −0.4) |
| 12 | 230 | 4 | 0.43 (t −0.9) |
| 14 | 230 | 4 | 0.42 (t −1.2) |
| 16 | 230 | 4 | 0.46 (t −0.6) |
| 18 | 230 | 4 | 0.58 (t +1.3) |
| 12 | 128 | 4 | 0.41 (t −1.4) |
| 12 | 250 | 4 | 0.62 (t +1.7) |
| 14 | 128 | 4 | 0.58 (t +1.2) |
| 12 | 230 | 6 | 0.36 (t −2.0) |
| **12** | **230** | **7** | **0.29 (t −3.3)** |
| 12 | 230 | 9 | 0.52 (t +0.4) |
| 10 | 230 | 7 | 0.62 (t +1.6) |
| 14 | 230 | 7 | 0.47 (t −0.4) |
| 12 | 200 | 7 | 0.45 (t −0.7) |

- Fast learning destroys a genome evolved without it (η 2^−6: the frozen copy wins everything).
- Slow learning with a slowly moving baseline helps even a genome that never had it.
- Chosen: η = 2^−12, λ = 230/256, β = 7 (the baseline averages over about 128 ticks).
- The landscape is noisy (SE about 0.07). In LL1 the genome evolves with learning, so it can adapt to these values.
  In arm LM the network can also scale the rate by 0 to 2.

Measures per seed (champion of the arm; 16 seeds × 2 mirrored games unless stated):
1. **Learning is used:** the champion with its plastic synapses frozen at zero (`-ablate freeze`) against an intact
   copy. Used if the frozen copy's share is below 0.5 with t < −3.
2. **Waste headroom,** as LT1: the champion with futile draws drawn again (gate_all, gate_eat) against an intact copy,
   averaged over epochs 1000–2000, in fresh arenas and in the run's own final world.
3. **Learning within life:** the futile share of agents aged 0–50 ticks against those aged 200–500 (probe). It should
   fall with age if they learn; in the controls it should not.
4. **Strength:** the champion against the five control champions.

Pass, per arm: learning is used in at least 2 of 3 seeds, and the waste headroom (gate_all, fresh arenas) is at most
0.70 in at least 2 of 3 seeds.
Kill, per arm: freezing costs nothing (|t| < 2) in every seed. Learning would then be a passenger, as in Geb.
Otherwise: no verdict; report it, with strength.

Next:
- If an arm passes, carry it to the W4f/W4g world (the main line), against W4f arm J.
- Then evolve the rule's form (DiscoRL, Oh et al., Nature 2025: learning rules discovered by meta-learning).

LL1 seeds: arms LR and LM stopped at seeds 1 and 2 to free the cores for LL1c (owner's options, 2026-11-27; the
decision timer expired with the recommendation applied).
- LM seed 2 then reversed the premise: freezing its champion is catastrophic (share 0.0007, t −1615). Seed 1 had
  gone the other way (0.60, t +2.4).
- So LM's verdict needs seed 3. runs8/w1l_LM_s3 (paused at epoch 70) resumes after the last LL1c run has started.
- LR seed 3 stays unrun: its seed-1 champion loses to every non-learning W1b champion (share 0.001).

## LL1 verdict (2026-11-27 23:05 UTC; static W1b; docs/results/ll1/)

| Arm | Freeze: frozen copy's share (t), seeds 1, 2, 3 | Learning used (t < −3) |
|---|---|---|
| L | 0.68 (+3.0), 0.68 (+3.4), 0.82 (+6.8) | 0 of 3: freezing helps in all 3 |
| LR | 0.43 (−2.4), 0.64 (+2.2), seed 3 not run | 0 of 2 |
| LM | 0.60 (+2.4), 0.0007 (−1615), 0.46 (−0.6) | 1 of 3 |

- **LL1 fails.** No arm uses learning in 2 of 3 seeds.
- In the static world, lifetime learning is neutral or harmful. The L arm is clearly harmful: the frozen copy wins
  in every seed.
- One lineage (LM seed 2) evolved into dependence on learning. It is the exception.
- This is what the literature predicts for a world without change (LL1c, Why). It is the control for LL1c. There,
  in a world whose food values change between generations, freezing is fatal in all 9 learning runs.

## LL2: an evolved reward (designed 2026-11-27; runs only if LL1 shows learning neutral or harmful)

Why:
- In the first LL1 result (arm L, seed 1), learning is neutral in the champion's own world and slightly harmful in
  fresh arenas.
- Learning raised attacks (0.19 of actions against 0.13 frozen), because a hit pays 20 energy at once.
- In a world of rivals that copy the same strategy, the extra fights cost the lineage agents.
- A reward of the agent's own energy does not track the lineage's fitness.

In animals the reward system is itself evolved, so what feels good tracks fitness rather than raw intake:
- the optimal reward problem: Singh, Lewis and Barto 2009, "Where do rewards come from?"; Singh et al. 2010, IEEE
  TAMD;
- evolved intrinsic rewards, with individual learning inside lives and the reward evolving across them, let
  cooperation arise where selfish learners fail (Wang et al., AAMAS 2019, arXiv:1811.05931);
- reward functions found by evolutionary meta-optimisation (arXiv:2512.13399, 2025) or black-box meta-learning
  (arXiv:2407.21546).

Mechanism (`-life_rew 1`; logit 14 in worlds without matter):
- The reward becomes the energy change plus logit_r/8 energy units, an internal reward the network outputs from what
  it perceives and remembers.
- The genome (the network's weights) shapes it, so evolution decides what the agent finds rewarding beyond energy:
  for example, a sense that fighting is bad when rivals are many, or that being near kin is good.
- It starts close to the energy reward: an untrained network's logit_r is noise around zero.
- Ablation `-ablate norew`: A learns from energy only.

Measures and pass/kill (W1b; on the best LL1 arm; 3 seeds × 2000 epochs):
1. Learning used: freeze costs (share < 0.5, t < −3) in at least 2 of 3 seeds.
2. The evolved reward is what makes it pay: `norew` also costs (t < −3) in at least 2 of 3 seeds.
3. What it rewards: the sensitivity of logit_r to each input (tools/w/sens on logit 14).
4. Waste headroom and strength, as LL1.

Pass: 1 and 2 hold.
Kill: freezing costs nothing in every seed (|t| < 2).


## LL1c: the decisive test, learning where it can pay (protocol fixed 2026-11-27, before the runs)

Why (the rethink of 2026-11-27):
- LL1 runs in W1b, a static world. The literature this plan cites predicts no benefit of plasticity there: it pays
  only in proportion to change faster than evolution and slower than a lifetime (WORLD.md, Le 2026; Stephens 1991).
- Learning is favoured only if it pays off within the agent's lifetime; otherwise evolution hard-codes a heuristic
  (Lange and Sprekeler, "Learning not to learn", arXiv:2010.04466).
- The artifacts world fails that test for individuals. Agents speak to a ready artifact about 4 times per life, so
  perfect individual memory recovers only about a quarter to a third of the value of knowing the law, even for a
  single key. Lineage-pooled memory recovers about 95% (ARTIFACTS.md, value of information).
- The design that made learning evolve in the laboratory: substrate value that changes between generations, with
  many choices per life (Mery and Kawecki 2002, PNAS 99:14274; Dunlap and Stephens 2014, PNAS).

World (`-kinds 4 -kind_bad 1 -kind_era 1000 -kind_tox 1` on W1b; corrected 2026-11-27 before any scored run, see below):
- Each cell's food is of one of 4 kinds, fixed per cell and drawn from the seed.
- In each era of 1,000 ticks (two maximum lifespans), exactly one kind is toxic. Eating it costs the energy it would
  have given. A naive eater keeps about half its net intake.
- Agents see the kind of the food underfoot and in front as one-hot lines (entries 53–60).
- Every eat is a trial with an immediate reward. Agents eat on about 40–50% of ticks.
- Evolution cannot hard-code the values: they change every 5 epochs, and the era is not observable.

Correction (2026-11-27, 17:00 UTC). The first setting was not viable from random founders:
- That setting was `-kinds 2 -kind_era 1000 -kind_tox 1`, each kind toxic with probability 1/2.
- Its E seed 1 had 44 reseeds in 200 epochs (guard limit 10; W1b: 0) and was stopped at epoch 342. It is kept as
  runs8/w1k0_E_s1 and not scored.
- Naive agents ate poison on about half their eats in most eras and starved.
- With toxic food giving nothing instead (`-kind_tox 0`), knowledge is worth nothing: the oracle copy loses (0.07).
- Viability check from random founders (E arm settings, 250 epochs; reseeds):

| World | Seed 1 | Seeds 2, 3 |
|---|---|---|
| kinds 2, each toxic w.p. 1/2, tox 1 | 60 (stopped) | |
| kinds 3, one toxic, tox 1 | 11 | |
| kinds 4, one toxic, tox 2 | 18 (guard trips) | |
| **kinds 4, one toxic, tox 1** | **3** | **0, 2** |

Calibration (value of information, W1b champion runs8/w1b_hist_s1 epoch 2000 against itself, 16 seeds × 2, A modified):
- **oracle:** never eats toxic food.
- **own memory:** refuses a kind its own last eat of it found toxic, retesting 1 time in 16. This is the perfect
  individual learner.
- **lineage memory:** the same with one memory per lineage.
- A refused eat becomes a forward move. Standing still on the food made the W1b genome stay on the cell.

| World | Oracle | Own memory | Lineage memory | Naive toxic share of eats |
|---|---|---|---|---|
| **kinds 4, one toxic, tox 1** | 0.99 (t 134) | **0.97 (t 56)** | 0.96 (t 15) | 0.24 |
| kinds 3, one toxic, tox 1 | 0.998 | 0.999 | 0.997 | 0.33 |
| kinds 4, one toxic, tox 2 | 0.9997 | 0.999 | 0.999 | 0.23 |

- Earlier settings, with refusal as standing still: kinds 2 (each toxic w.p. 1/2), tox 1: oracle 0.91, own memory
  0.83. Kinds 4, tox 2: 0.83 and 0.64.
- In the chosen world, a perfect individual learner gets almost all of what knowledge is worth.

Learner check (the same champion, learning B against frozen A, `-ablate freeze`, chosen world):

| Learner | Toxic share of eats, B by age 0–99 → 400+ (A frozen: 0.23–0.25) | Frozen A's share (without / with `-life_mod 1`) |
|---|---|---|
| LL1 rule, features only (eta 12) | 0.237 overall | 0.34 (t −2.8) / – |
| + kind lines at the same rate | 0.237 overall | 0.42 (t −1.1) / 0.24 (t −5.4) |
| + kind lines at 2^-9 | 0.227 → 0.218 | 0.38 (t −2.1) / 0.25 (t −5.1) |
| **+ kind lines at 2^-7** | **0.216 → 0.169** | **0.38 (t −1.7) / 0.21 (t −5.4)** |
| + kind lines at 2^-5 | 0.161 → 0.116 | 0.91 (t +19) / 0.67 (t +2.3) |

- At one rate, a one-hot line carries |x|² = 4,096 against about 160,000 for the 64 features. A toxic eat moves the
  eat logit through its kind line by about 0.08 units (16 units halve the probability), so the rule learns "eat more
  or less", not "avoid this kind".
- A separate rate for the sensory synapses (`-life_xeta`) lets the agent learn the food law within its life. At 2^-7
  the toxic share falls with age to 0.17.
- Faster rates also teach agents to attack much more. At 2^-5 the learning copy loses (0.91 to the frozen one).
- With the fixed genome, the benefit of learning comes mostly from fewer attacks (0.12–0.13 against 0.16–0.19).
  Whether evolution makes the food law the thing that is learned is part of what the runs show (measure 3).

Arms (seeds 1–3, 2,000 epochs, the LL1 settings otherwise; binary runs8/oee_w_v29):
- **E:** no learning (control; the recurrent state can still adapt within a life).
- **L:** LL1's L (eta 12, lam 230, beta 7), with the plastic synapses also reading the kind lines at rate 2^-7
  (`-life_in 53,61 -life_xeta 7`).
- **LM:** L with the evolved neuromodulator (`-life_mod 1`). Evolution can gate when learning happens, i.e. prepared
  learning (Dunlap and Stephens 2014).
- **LL2:** L with the evolved reward (`-life_rew 1`, LL2 above). Added 2026-11-27 before the runs, when the owner's
  decision timer expired with the recommended option applied: LL1 has come out neutral or harmful, LL2's
  precondition. If L fails and LL2 passes, the energy reward was the problem rather than the learning rule
  (Ackley and Littman 1991, evolved evaluation network).

Measures (final champions, fresh held-out arenas, 16 seeds):
1. **Learning is used:** freeze A's plasticity against an intact copy (L, LM).
2. **Learning helps evolution:** the arm's champion against the E champion of the same seed.
3. **The learning curve:** toxic share of eats by age within an era, and after an era change, against E and against
   the frozen copy.
4. **Control:** the same freeze test in static W1b (LL1). The freeze cost should be larger here.

Pass, per arm: freezing costs fitness (share < 0.5, t < −3) in at least 2 of 3 seeds, and the champion beats E's
(share > 0.5, t > 2) in at least 2 of 3 seeds.
- LL2 additionally: `-ablate norew` (learning from energy only) also costs (t < −3) in at least 2 of 3 seeds. That
  shows the evolved reward is what makes learning pay.

Kill: freezing costs nothing (|t| < 2) in every seed of L, LM and LL2. Then the three-factor learner does not pay even
where a perfect individual learner wins 0.97, so the learner is the bottleneck, not the world.

Viability (stated 2026-11-27 18:00 UTC, before any learning arm was scored; the project's guard, WORLD.md W3.x):
- A run whose reseeds rise by more than 10 within 200 epochs is stopped and reported, not scored.
- The world itself is viable: E seed 1 had 10 reseeds in 2,000 epochs, and the calibration seeds 3, 0 and 2 in 250.
- So a non-viable learning run counts as a failure of its arm at that seed. Its learner cannot keep the lineage
  alive.
- LM seed 1 tripped the guard at epoch 728: 18 reseeds between epochs 450 and 650, 25 in all. It was stopped there
  (runs8/w1k_LM_s1/NONVIABLE).
- LL2 seed 1 tripped it at epoch 912: 6 reseeds by epoch 400, 49 by epoch 900. It was stopped at 919
  (runs8/w1k_LL2_s1/NONVIABLE).

Viability rule withdrawn (2026-11-27 19:08 UTC; owner's decision timer expired with the recommended option applied):
- The 18:00 rule rested on the world being viable. That premise failed at 18:45, when the E arm's seed 3 tripped the
  guard (13 reseeds by epoch 830). Then L seed 2 tripped it too (20 by epoch 467).
- Collapses come after epoch 250, beyond the viability check, and in every arm.
- So for LL1c the guard is a reported measure, not an exclusion. All 12 runs go to epoch 2,000 and are scored by the
  pass/kill above. Reseeds per run are reported beside the scores.
- The stopped runs resume from their checkpoints: E s3 at 830, L s2 at 467, LM s1 at 747, LL2 s1 at 919. The
  runs8/RUN/VIAB_NOTE files record the trips.
- The scores are fresh-arena tournaments from founders, so a champion's score does not depend on its run's
  population history.
- This change comes after one learning result (L seed 1 passed). Its cause, the control collapsing, is independent of
  that result.
- Lesson for later worlds: check viability over at least 1,000 epochs, not 250.

Runs: runs8/w1k_{E,L,LM,LL2}_s{1,2,3}, queued in that order by seed once the viability check passes. Scored by
tools/w/ll1c_eval.sh into docs/results/ll1c/.

## LL1c verdict (2026-11-27 21:45 UTC; all 12 runs at epoch 2,000; docs/results/ll1c/)

Fresh held-out arenas, 16 seeds × 2. "Freeze": the champion with its plasticity frozen against an intact copy.
"Against E": the champion against the E champion of the same seed. Toxic share by eater age, born in the current
era. Reseeds: in the run's 2,000 epochs.

| Run | Freeze: frozen share (t) | Against E (t) | Toxic share intact / frozen | Intact toxic, age 0-99 -> 400+ | Against epoch 1000 (t) | Reseeds |
|---|---|---|---|---|---|---|
| E s1 | - | - | 0.254 / - | 0.246 -> 0.248 | 0.844 (8.60) | 10 |
| E s2 | - | - | 0.248 / - | 0.231 -> 0.263 | 0.549 (0.73) | 0 |
| E s3 | - | - | 0.251 / - | 0.232 -> 0.261 | 0.778 (6.30) | 15 |
| L s1 | 0.042 (-52.33) | 0.797 (5.91) | 0.186 / 0.291 | 0.210 -> 0.114 | 0.719 (4.26) | 18 |
| L s2 | 0.233 (-8.64) | 0.279 (-3.13) | 0.175 / 0.271 | 0.192 -> 0.116 | 0.593 (2.65) | 28 |
| L s3 | 0.199 (-9.21) | 0.294 (-3.26) | 0.156 / 0.262 | 0.191 -> 0.090 | 0.549 (1.14) | 48 |
| LM s1 | 0.038 (-74.22) | 0.584 (1.01) | 0.167 / 0.285 | 0.180 -> 0.102 | 0.478 (-0.59) | 133 |
| LM s2 | 0.122 (-11.08) | 0.617 (1.48) | 0.150 / 0.270 | 0.189 -> 0.058 | 0.546 (0.84) | 38 |
| LM s3 | 0.078 (-28.06) | 0.341 (-2.30) | 0.162 / 0.272 | 0.194 -> 0.089 | 0.574 (1.64) | 44 |
| LL2 s1 | 0.153 (-8.44) | 0.647 (1.96) | 0.174 / 0.277 | 0.199 -> 0.102 | 0.699 (5.72) | 90 |
| LL2 s2 | 0.067 (-33.53) | 0.665 (2.18) | 0.170 / 0.265 | 0.197 -> 0.106 | 0.649 (2.78) | 111 |
| LL2 s3 | 0.061 (-26.62) | 0.662 (2.80) | 0.174 / 0.271 | 0.203 -> 0.091 | 0.603 (1.98) | 15 |

By the protocol:
- **L fails.** Freezing costs in 3 of 3 seeds, but it beats E in 1 of 3 (seed 1, t 5.9; seeds 2 and 3 lose,
  t −3.1 and −3.3).
- **LM fails.** Freezing costs in 3 of 3, but it beats E in 0 of 3 (t 1.0, 1.5, −2.3).
- **LL2 passes the main criteria:** freezing costs in 3 of 3, and it beats E in 2 of 3 (t 2.2 and 2.8; seed 1 has
  t 1.96).
  - It fails its own criterion: `norew` costs nothing in any seed (t 0.7, +2.05, −1.5).
  - So the evolved reward is not shown to be why it pays.
- **The kill is not met.** Freezing collapses every learning champion (frozen share 0.04–0.23, t −8 to −74).

What holds in every learning run:
1. **Learning is used.** Without it, every learning champion loses to its own intact copy.
2. **Agents learn the food law during life.** The toxic share of eats falls from 0.18–0.21 at ages 0–99 to
   0.06–0.12 at 400+. Frozen copies stay at 0.26–0.29 and the E lineages at 0.25–0.26, flat with age.
   - Learning lineages learn while E lineages never do: the recurrent state alone did not find the law.
   - Frozen learners eat more poison than chance. Their genomes now expect learning to correct them.
3. **The contrast with the static world is the predicted one.** In LL1 (W1b, no change), freezing the L arm helps
   in 3 of 3 seeds (frozen share 0.68, 0.68, 0.82). Here it is fatal.

Why learning does not reliably beat evolution alone (head to head, seeds 1 and 3; L against E):

| Per thousand agent-ticks | Seed 1: L / E (L wins 0.80) | Seed 3: L / E (L loses 0.29) |
|---|---|---|
| Toxic share of eats | 0.165 / 0.280 | 0.157 / 0.262 |
| Starved | 3.75 / 6.80 | 3.65 / 5.44 |
| Died of age | 1.42 / 0.69 | 1.29 / 0.88 |
| Births | 5.08 / 7.22 | 4.47 / 6.37 |
| Reproduce actions (share of all actions) | 0.019 / 0.025 | 0.012 / **0.093** |

- Learning does its job in both seeds: less poison, about half the starvation, more agents reaching old age.
- The E lineage of seed 3 instead evolved to breed fast. It makes almost 8 times as many reproduce attempts, starves
  more and wins the biomass.
- Reproduction is innate (not plastic), so this is a lineage's evolved strategy, an r against K trade-off. Learning
  pays with long lives; fast breeding is the alternative.
- With 3 seeds, which strategy a lineage happens to evolve decides much of "against E".
- Learning lineages are also less stable: 15–133 reseeds against 0–15 for E.

Conclusions:
- The question this test was built for, whether our evolved agents can use lifetime learning where it pays, has a
  yes. Learning is evolved into dependence, and the food law is learned within lives.
- The stronger claim, that learning makes better lineages than evolution alone, holds only for LL2 (2 of 3) and is
  fragile. With n = 3 it can be decided by unrelated evolved strategies.
- The evolved neuromodulator (LM) and the evolved reward (LL2) are not shown to help. LM is least stable (38–133
  reseeds). LL2's reward ablation is neutral.
- The SL1 gate (SOCIAL.md: learning used, and freezing it costs fitness) is met in the LL1c world.


## LL1c replication (protocol fixed 2026-11-27 22:03 UTC, before the runs)

Chosen when the owner's decision timer expired with the recommended option applied. Reason: "learning beats
evolution alone" rests on 3 seeds and is partly decided by unrelated evolved strategies (breeding rate).

Runs: arms E, L and LL2, seeds 4–6 (runs8/w1k_{E,L,LL2}_s{4,5,6}). Same flags and binary (runs8/oee_w_v29) as seeds
1–3. LM is not repeated: it beat E in 0 of 3.

Measures: as LL1c, scored by tools/w/ll1c_eval.sh, plus:
- **LL2 against L** head to head in each of the 6 seeds (both learning).
- **Pooled share against E** for each arm: the mean over its 6 seeds of the per-seed share, with a one-sample t over
  seeds against 0.5.

Pass, per arm, over seeds 1–6:
- freezing costs (t < −3) in at least 4 of 6 seeds;
- and it beats E (t > 2) in at least 4 of 6.

Secondary: the pooled share against E is above 0.5, with t > 2 over seeds.

LL2 is better than L if it wins the head to head (t > 2) in at least 4 of 6 seeds.

Kill of "learning beats evolution alone": both L and LL2 beat E in at most 2 of 6 seeds.

Viability: reported, not an exclusion (as decided for LL1c at 19:08).

## LL1c replication verdict (2026-11-28 01:30 UTC; seeds 1–6; docs/results/ll1c/)

| Run | Freeze: frozen share (t) | Against E (t) | Toxic share intact / frozen | Intact toxic, age 0-99 -> 400+ | Against epoch 1000 (t) | Reseeds |
|---|---|---|---|---|---|---|
| E s1 | - | - | 0.254 / - | 0.246 -> 0.248 | 0.844 (8.60) | 10 |
| E s2 | - | - | 0.248 / - | 0.231 -> 0.263 | 0.549 (0.73) | 0 |
| E s3 | - | - | 0.251 / - | 0.232 -> 0.261 | 0.778 (6.30) | 15 |
| E s4 | - | - | 0.249 / - | 0.246 -> 0.242 | 0.851 (6.90) | 7 |
| E s5 | - | - | 0.255 / - | 0.245 -> 0.262 | 0.709 (5.28) | 3 |
| E s6 | - | - | 0.257 / - | 0.255 -> 0.251 | 0.998 (665.22) | 4 |
| L s1 | 0.042 (-52.33) | 0.797 (5.91) | 0.186 / 0.291 | 0.210 -> 0.114 | 0.719 (4.26) | 18 |
| L s2 | 0.233 (-8.64) | 0.279 (-3.13) | 0.175 / 0.271 | 0.192 -> 0.116 | 0.593 (2.65) | 28 |
| L s3 | 0.199 (-9.21) | 0.294 (-3.26) | 0.156 / 0.262 | 0.191 -> 0.090 | 0.549 (1.14) | 48 |
| L s4 | 0.091 (-42.61) | 0.162 (-5.40) | 0.170 / 0.267 | 0.192 -> 0.105 | 0.714 (6.21) | 110 |
| L s5 | 0.228 (-5.41) | 0.837 (6.81) | 0.192 / 0.270 | 0.221 -> 0.119 | 0.618 (1.79) | 8 |
| L s6 | 0.298 (-6.47) | 0.006 (-309.35) | 0.168 / 0.271 | 0.206 -> 0.081 | 0.768 (7.22) | 44 |
| LM s1 | 0.038 (-74.22) | 0.584 (1.01) | 0.167 / 0.285 | 0.180 -> 0.102 | 0.478 (-0.59) | 133 |
| LM s2 | 0.122 (-11.08) | 0.617 (1.48) | 0.150 / 0.270 | 0.189 -> 0.058 | 0.546 (0.84) | 38 |
| LM s3 | 0.078 (-28.06) | 0.341 (-2.30) | 0.162 / 0.272 | 0.194 -> 0.089 | 0.574 (1.64) | 44 |
| LL2 s1 | 0.153 (-8.44) | 0.647 (1.96) | 0.174 / 0.277 | 0.199 -> 0.102 | 0.699 (5.72) | 90 |
| LL2 s2 | 0.067 (-33.53) | 0.665 (2.18) | 0.170 / 0.265 | 0.197 -> 0.106 | 0.649 (2.78) | 111 |
| LL2 s3 | 0.061 (-26.62) | 0.662 (2.80) | 0.174 / 0.271 | 0.203 -> 0.091 | 0.603 (1.98) | 15 |
| LL2 s4 | 0.212 (-5.85) | 0.132 (-6.23) | 0.201 / 0.281 | 0.224 -> 0.125 | 0.614 (2.64) | 149 |
| LL2 s5 | 0.190 (-15.00) | 0.716 (2.61) | 0.166 / 0.256 | 0.183 -> 0.111 | 0.648 (2.40) | 72 |
| LL2 s6 | 0.167 (-10.19) | 0.012 (-205.27) | 0.174 / 0.278 | 0.188 -> 0.102 | 0.560 (1.27) | 136 |

Summary (tools/w/ll1c_rep.sh; head to head: LL2 as A, L as B learning from energy only):

```
L: freeze t<-3 in 6 of 6; beats E (t>2) in 2 of 6; pooled share against E 0.396 (sd 0.343, t -0.74 over 6 seeds)
LL2: freeze t<-3 in 6 of 6; beats E (t>2) in 3 of 6; pooled share against E 0.473 (sd 0.313, t -0.21 over 6 seeds)
LL2_vs_L	s1	0.3400	0.0586	-2.73	5/16
LL2_vs_L	s2	0.7463	0.0679	3.63	13/16
LL2_vs_L	s3	0.7047	0.0352	5.81	15/16
LL2_vs_L	s4	0.5300	0.0450	0.67	7/16
LL2_vs_L	s5	0.3642	0.0689	-1.97	4/16
LL2_vs_L	s6	0.7631	0.0256	10.27	16/16
```

By the protocol:
- **L fails.** It beats E in 2 of 6 seeds (the kill for L).
- **LL2 fails.** It beats E in 3 of 6; 4 were needed.
- **LL2 is not shown better than L.** It wins the head to head in 3 of 6 seeds (t > 2).
- **The kill of "learning beats evolution alone" is not met formally** (LL2 has 3 of 6). But neither arm has an
  average advantage: pooled shares against E are 0.40 (t −0.74) and 0.47 (t −0.21).
- **Learning is used in all 12 learning runs.** Freezing costs in 12 of 12 (t −5 to −74).
- **The food law is learned within life in all 12.** Toxic share 0.18–0.22 at ages 0–99 falls to 0.08–0.13 at 400+.
  It stays flat at 0.25–0.26 in all 6 E lineages.

Why learning does not make better lineages (the hiding effect). Each learning champion frozen, against the E champion
of its seed (docs/results/ll1c/hiding.txt):

```
s1 L frozen vs E: 0.2239 (t -5.05)
s1 LL2 frozen vs E: 0.0743 (t -18.48)
s2 L frozen vs E: 0.0402 (t -120.68)
s2 LL2 frozen vs E: 0.0447 (t -94.96)
s3 L frozen vs E: 0.0714 (t -17.62)
s3 LL2 frozen vs E: 0.0562 (t -14.07)
s4 L frozen vs E: 0.0172 (t -72.41)
s4 LL2 frozen vs E: 0.0219 (t -44.53)
```

- The innate genome of a learning lineage is far weaker than an E genome (0.02–0.22). With learning it climbs back
  to about E's level (0.40–0.47 on average).
- Learning stands in for genetic adaptation instead of adding to it. This is the hiding effect: learning makes
  genotypes look alike to selection and slows the evolution of innate competence (Paenke, Sendhoff and Kawecki 2007,
  Am. Nat.; Paenke et al. 2009, Artif. Life 15:227).
- The learning lineages also gain less against their own epoch-1000 ancestors: 0.55–0.77 against E's 0.55–0.998.
- Learning lineages are less stable as populations: 8–149 reseeds against 0–15.
- Where an E lineage evolved fast breeding, it beat learners that ate better (seed 3, verdict above).

Conclusions:
- Our evolved agents do use lifetime learning where it can pay, and they learn the world's changing law within their
  lives. That was not true before LL1c.
- Individual learning that restarts from zero every life does not make better lineages than evolution alone in this
  world. Evolution lets learning carry the load and lets innate competence decay.
- Two kinds of next step follow from the measured cause rather than from a new mechanism:
  - inheritance of what was learned (SL1 V, teaching at birth, built), so a newborn does not restart from zero;
  - or the long-run test of genetic assimilation (the Baldwin effect may need longer than 2,000 epochs).

Hiding over time (docs/results/ll1c/hiding_trend.txt). Each frozen learning lineage was played against the E lineage of
the same seed at the same epoch (500, 1,000, 1,500, 2,000):
- In 10 of 12 lineages the gap stays or widens over the run. For example, L seed 3 goes from 0.23 to 0.06 and LL2
  seed 1 from 0.32 to 0.05. Innate competence falls further behind.
- The exception is L seed 5 (0.04 to 0.57). It is also the lineage that beat E most clearly (0.84).
- So there is no sign of genetic assimilation within 2,000 epochs. Longer runs are unlikely to close the gap in most
  seeds.

