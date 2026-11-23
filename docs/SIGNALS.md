# Signalling in Geb agents

Question: do agents use the neighbour-signal channel (inputs listen to neighbours' outputs by bit prefix)?
Method follows Lowe et al., "On the Pitfalls of Measuring Emergent Communication" (arXiv:1903.05168): positive
signalling holds by construction in Geb (the outputs that drive an agent's actions are what neighbours hear), so
the question is positive listening, which needs an intervention.
Tools: `tools/signals` (replay recording per agent-step what was heard, by signal class, and what was done),
`World::set_deaf` (analysis only), `tools/signals/analyse.py`.

## l4Ln_s1 checkpoint at 1,140,000 (L68, no locks), 1500 steps, normal vs deaf (same random streams)
- I(heard; did) = 0.648 bits of 1.497 (43%), shuffled null ~0. Confounded: agents whose genomes grew no inputs
  always "hear nothing" and also act differently, so heard is partly a genotype proxy. Not evidence by itself.
- Intervention (causal): deaf agents attack 7.6% less, bond 7.8% less, reproduce 5.6% less, move 2x more;
  warriors 1336 vs 851 (-36%); births +4.1%; population 3457 vs 3357 (-2.9%) after 1500 steps.
- Reading: positive listening is real. Hearing neighbours keeps agents in place, raises attack and bonding and
  sustains the warrior role. The channel carries action broadcasts, not a language.
- Next: per-agent CIC (deafen a random half, compare matched agents) to remove the genotype confound; repeat on
  the lock run and a second seed.
Data: `docs/results/signals/`.

## Per-agent causal test (random half deaf by id hash, same world; label not inherited)
Same checkpoint, 1500 steps. Among agents with input neurons (97%):

| action | hearing | deaf | diff |
|---|---|---|---|
| reproduce | 0.760 | 0.645 | +0.115 |
| attack | 0.797 | 0.668 | +0.130 |
| turn | 0.773 | 0.685 | +0.087 |
| bond | 0.796 | 0.665 | +0.131 |
| move | 0.019 | 0.033 | -0.014 |

- Random assignment removes the genotype confound: hearing causally raises reproduce/attack/bond by 11-13 points.
- Survival: hearing share among agents with inputs 0.503 -> 0.603 (+0.099). Control (same labels, nobody deaf):
  0.492 -> 0.507 (+0.015). Labels are not inherited, so the excess +0.084 is differential survival: hearing
  agents live longer. Single checkpoint, single seed.
- Conclusion: the signalling channel is used and selected for. It carries action broadcasts that coordinate
  attack, bonding and reproduction within colonies; it is not a language.
