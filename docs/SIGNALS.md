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
