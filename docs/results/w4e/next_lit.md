# After W4e: literature on tending a fixed place (searched 2026-11-27; every item opened at the URL given)

Question: agents harvest homestead farms but never learn to keep feeding them. What do agents or the learner need?
Constraints: no pretrained models, no demonstrations, fitness from the world only.

| Source | Finding | Use here |
|---|---|---|
| Floreano and Mondada 1996, IEEE Trans. SMC-B 26(3), doi:10.1109/3477.499791 | A 5-hidden-unit recurrent net evolved on a real robot returned to a charger only when its battery input fell to ~10%; the charger had a light visible from anywhere; hidden units formed heading-dependent maps | The precedent had a beacon, an input reporting the need, and a payoff to the actor: candidate variables to add one at a time |
| Aharonov-Barki, Beker and Ruppin 2001, Neural Computation 13(3), doi:10.1162/089976601300014529 | Evolved foragers grew a command neuron; with position sensors it became place-like, without them evolution built a short-term memory instead | A position input changes which solution evolution finds; A/B it |
| Vickerstaff and Di Paolo 2005, J. Exp. Biol. 208(17), doi:10.1242/jeb.01772 | A GA evolved landmark-free homing that integrated compass and speed inputs | Without a compass, homing must be built from counted turns in int8 recurrence; a world-computed heading or home vector is the smallest change |
| Wijmans et al. 2023, ICLR, arXiv:2301.13261 | Blind LSTM agents with egomotion reached ~95% success, memoryless ~0%; maps formed in memory over ~1,000 steps | A position signal still needs long memory; probe hidden states for farm direction |
| Lin, Yiu and Leibold 2026, ICLR, arXiv:2510.09951 | Place fields emerge from egocentric views; the memory architecture decides under sparse cues | A 5x5 window on uniform land is a sparse cue |
| Hamon, Sánchez-Fibla, Moulin-Frier and Solé 2026, arXiv:2605.22256 | Farming among PPO agents needed a high discount, scarce wild food (abundant foraging is a trap), groups under ~5 (cheaters), and then locked in | The homestead's free store and fresh irrigation are the wild-food trap |
| Hamon, Nisioti and Moulin-Frier 2023, arXiv:2302.09334 | Without resets, evolved agents included sustainable foragers; resets cut the feedback | Fresh farms every epoch mean neglect is never inherited: carry fertility across epochs |
| Salimans et al. 2017, arXiv:1703.03864 | ES scores whole episodes and tolerates long horizons and delayed rewards | Delay inside 600 ticks is not the obstacle by itself; signal-to-noise is |
| Ni, Ma, Eysenbach and Bacon 2023, NeurIPS, arXiv:2307.03864 | LSTM (hidden 128) fails pure-memory T-mazes from ~250 steps; Transformers to ~1,500 | Remembering where and when to feed over hundreds of ticks is where small recurrent cores fail; make farm hunger visible or test a larger state |
| Lehman, Chen, Clune and Stanley 2018, GECCO, arXiv:1712.06568 | ES maximises the mean fitness of the perturbation cloud and falls off narrow high-fitness paths at high variance | Reliable whole-lineage feeding is a narrow path; lower sigma or rank once feeders exist |
| Colas, Huizinga, Madhavan and Clune 2020, GECCO, arXiv:2003.01825 | ME-ES (ES inside a MAP-Elites archive) escaped deceptive tasks where fitness-only ES stayed trapped | Archive keyed on feeds per agent-tick and on time on own land after the store runs out |
| Wang, Lehman, Clune and Stanley 2019, arXiv:1901.01753 (POET) | Co-generated environments in a not-too-easy band, with transfer, solve what a hand-built curriculum cannot | Generate farm-world variants under such a band |

Ranked next steps (from the search, to be fixed with kill criteria before any code):
1. World: no free store and no fresh irrigation (or fertility carried across epochs), farms closed to other
   lineages. Separates a missing incentive from a missing capability.
2. Agent: a world-computed cue (a farm gradient visible in the 5x5 view or an egocentric home vector) plus visible
   farm hunger; A/B against no cue, judged on held-out arenas.
3. Learner: ME-ES archive around EGGROLL keyed on feeding and on time on own land, with small-sigma phases.
