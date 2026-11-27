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
epoch 2000), learning on against learning off. The calibration and the chosen values are recorded below, before the
runs start.

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
