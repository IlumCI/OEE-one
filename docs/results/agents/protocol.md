### Agent study protocol (fixed 2026-11-27, before the measurements)

Question (owner, 2026-11-27): are the agents themselves flawed, or carrying broken mutations?

Genomes: the champion at the last epoch of each finished W4 run (W4d-W4f; W4g when it ends), the champion at half
that epoch, and the initial genome egg_0 as the no-evolution control. Each genome plays in self-play (A = B) in its
training world (wflags.sh). Tools: oee_w -probe, -mask, -mask_mode, -ablate gate_eat / gate_all;
tools/w/probe.py, tools/w/eggstat.py, tools/w/agents.sh.

Measures and reading rules:
1. Broken network (structure): weights at the int8 limit, hidden units that are constant, saturated (> 90% of steps
   at |h| >= 127) or tau-dormant (Sokar et al. 2023, tau 0.1), effective rank (Kumar et al. 2021), action logits
   that never change. Broken if any of these affects more than 10% of units or weights.
2. Drift (mutational load): correlation of successive weight changes, against null parameters (input entry 127 is
   always 0; logits 10-11 are unused). Selection on single weights would show a positive correlation above the null.
3. Input use (lesions, champion with one observation group removed against an intact copy, 16 seeds x 2): a group is
   used if mean_lshareA < 0.5 with t < -3 under the shuffle lesion (each agent sees another agent's values, which
   keeps the input distribution). A zero lesion that costs fitness while the shuffle lesion does not is an
   off-distribution effect, not use of the information (zero vs resample ablation; Zhang and Nanda 2023,
   arXiv:2309.16042).
4. Behavioural flaws (probe): actions with no effect (futile), choices that ignore what the agent sees (e.g. eating
   as often with no food underfoot as with food), choices never made.
5. Headroom of a flaw (gates): the champion whose futile draws are drawn again (gate_eat: futile eats only; gate_all:
   every futile action) against an intact copy. A missed easy gain if mean_lshareA > 0.55 with t > 3: the learner
   did not find an improvement that needs only a condition on an input the agent already has. No headroom if
   |t| < 2: the flaw costs nothing in this world.
