# Signal outputs and lifetime plasticity

Follows docs/SIGNALS.md. There, every Geb output is both an action and a signal, so what
neighbours hear is a broadcast of what an agent does. This step adds a channel with no
physical effect (cheap talk), and learning within a lifetime.

## Signal outputs (`-signals`, Params::plastic bit 1)

- An output whose character starts with `0011` drives no action. It is excluded from every
  action sum, including unbond (`001`) and lock keys.
- Neighbours still hear it through the usual Geb matching: an input hears an output when the
  input's character, minus its side bit, is a prefix of the output's character.
- A signal therefore costs nothing and does nothing, except through its effect on whoever hears
  it. Whether it gets used is up to evolution.

## Plasticity (`-plastic`, Params::plastic bit 2)

Each excitatory link carries a weight w, which starts at 1 at birth. A node's drive becomes
sum w_i * pre_i.

After every network update, each agent applies its own heritable rule:

    dw = eta * (A*pre*post + B*pre + C*post + D)      pre, post in [0,1]

This is the generalised Hebbian "ABCD" rule:
- Najarro & Risi, arXiv:2007.02686.
- Yaman et al., arXiv:1904.01709: evolved interpretable local rules.
- Pedersen & Risi, arXiv:2104.07959: rule sharing.
- arXiv:2507.11566 (2025): one evolved ABCD rule shared by a swarm, with heterogeneity coming
  from each agent's experience.

Departures from those papers:
- **The rule evolves inside an open-ended world.** It is not meta-learned against a task.
  A, B, C and D are in eighths (-8..8). eta = 2^-g with g in 0..7, and g = 0 means no learning.
- **Founders get random rules.** A child takes each coefficient from either parent, then each
  coefficient takes a +-1 step with probability 1/5. Learned weights are not inherited.
- **Synaptic normalisation.** After the update, weights are clamped to [0,4], and each node's
  incoming weights are rescaled to mean 1.

Why normalisation: without it, evolution selected D > 0. Weights then ran to the cap within a
lifetime, which is a gain increase, not learning. With normalisation, learning can only
redistribute a node's drive among its inputs. In a 30k-step test, weights stayed within 10–20%
of 1.

## Checkpoints and tests

- Checkpoint magic V10 stores Params::plastic, each agent's rule and its link weights. Older
  checkpoints load with plastic = 0.
- With both bits off, results are bit-identical to before; all 12 ctest checks pass.
- tests/check_plastic.sh: a run resumed from a checkpoint matches an uninterrupted one, and
  2 threads match 1.
- Log line `time: P`, values per agent:
  - learners: fraction with g > 0
  - eta_g: mean g
  - A..D: mean coefficients
  - wdev: mean |w - 1|
  - sig_out: fraction with a signal output
  - sig_on: fraction whose signal output is non-zero
- Analysis switch: `Ann::freeze` (not saved) stops all weight changes, for ablation.

## Experiment (runs6, plSP_* vs plS_*)

- L24, neurons 40, kin colonies, division of labour, senescence, counter RNG, 1M steps,
  seeds 1–2.
- Arms: signals + plasticity (SP), and signals only (S).
- The runs use nice 10, so the L4 scale test keeps priority.

Planned measurements:
1. Do the signal outputs get used? Track sig_out and sig_on over time, and the
   heard/did influence of `0011` signals measured as in SIGNALS.md.
2. Causal effect of learning: on SP checkpoints, compare behaviour and births with
   `Ann::freeze` on and off.
3. SP vs S: population, births, complexity and activity statistics.

## Interim results

### Freeze test at about 190k steps

Method:
- Checkpoints: plSP_s1 at 195k steps and plSP_s2 at 186k.
- Each replay runs 500 steps with learning on and again with learning frozen, for 12 paired
  reseeded replicates per checkpoint.
- Raw output is in docs/results/plastic/.

Results:
- **Births and population: no effect.**
  - s1: births +6 of about 49,900 (t +0.02).
  - s2: births +152 of about 46,600 (t +0.42).
- **Behaviour in s1**, where all agents learn: learning raises attack by 0.4 percentage points
  (t +2.4) and turn by 0.5 points (t +2.9). Reproduce rises 0.5 points, but at t +1.7 this is
  not clearly separate from noise.
- **Behaviour in s2:** nothing.
- **Signal outputs:** below 2% of agents in all four runs, with no trend.

At this stage the rule has spread (s1 has learners 1.0), but it is close to a passenger. It
changes behaviour slightly and fitness not at all.

### Freeze and signal-mute tests at about 485k steps

Method:
- **Freeze test:** same method as at 190k.
- **Mute test:** a new analysis mode, deaf 3, makes outputs starting `0011` inaudible to every
  listener. Every other output is still heard. It is run as `freeze.py ... mute`.
- Raw output is in docs/results/plastic/.

Results:

| Test | Births | Behaviour |
|---|---|---|
| Freeze, s1 | -516 of about 51,000 (-1.0%, t -1.7) | move +0.2 points (t +2.9); all others \|t\| < 1.7 |
| Freeze, s2 | -301 of about 48,600 (t -0.9) | nothing (all \|t\| < 0.8) |
| Mute, s2 | +253 (t +0.63) | nothing (all \|t\| < 1.5) |

- **Freeze:**
  - In s1, learning leans towards a small cost, though not significantly. Its only clear
    effect is slightly more moving.
  - The 195k behaviour effects (attack, turn) did not persist.
- **Mute:**
  - At this checkpoint, 12.6% of s2 agents had an active signal output during the replays.
  - The signal had risen from 0.2% to 7.5% over 40 minutes.
  - Muting it changes nothing, so it is sent but not used: it spreads by drift, with no
    receivers.
- **Consistency check:** the learning-on rows for s2 equal the heard rows exactly. That is
  expected, since both use the same checkpoint and seeds.

At 485k steps the two additions are still neutral: learning is near a passenger, and the
signal channel is unheard.

Why this might be, from the literature on the evolution of communication:
- Senders and receivers have to co-evolve before either pays.
- A channel with no effect until receivers exist sits at a fitness plateau.

Planned measurements continue to 1M.

### plSP_s2 at 600k: signal at its peak

Background:
- plSP_s2's signal rose from 0.2% to 17% of agents between 570k and 597k steps, the largest
  rise in any run so far.
- An earlier mute test, run on the 580k checkpoint, did not test this rise: the checkpoint
  predates it, and only 2% of agents were signalling during those replays.
- The tests below use the 600k checkpoint. Agents with a signal output were 11–12% of the
  population during these replays.

Mute test (heard minus muted):
- Births: -0.7% (t -1.5). Population: no change (t +0.1).
- All behaviours: |t| < 1.6.
- Signalling itself: -0.8 points (t -2.2). This is the only effect beyond noise. Agents send
  slightly less when signals are audible.

Freeze test (learning on minus frozen):
- Births: -0.85% (t -1.55).
- Reproduce rate: -0.4 points (t -2.0).

Freeze births across tests (learning on minus frozen):

| Checkpoint | Births |
|---|---|
| s1, 485k | -1.0% |
| s2, 485k | -0.6% |
| s2, 600k | -0.85% |

Learning lowers births in all three, but none is individually significant. Sign-consistent
across tests: a small cost. Signals are still not heard in any way that matters.
