# Tournaments: do later populations beat earlier ones?

Interim, 2026-11-25. The batch is still running; see §4.

## 1. Question

Our earlier measures (activity statistics, complexity) count genetic change, not ability. A tournament asks
directly whether a population from step t out-competes one from an earlier step of the same run.

## 2. Method (tools/tourney)

**Sampling.** Take 128 genomes from each of two checkpoints and place them alternately in the first 256 cells of
a fresh 32×32 arena, all as newborns:
- The arena uses the later run's rules: kin colonies, division of labour, senescence, counter RNG.
- It has no locks and no growth.
- The neuron limit is the larger of the two, unless set otherwise.

**Neutral marker (`Organism::mark`).**
- A uniparental tag, copied from the randomly chosen first parent.
- Under neutrality its share stays at 0.5 in expectation, whatever the genome lengths.
- The genome-share measure (`anc`) favours longer genomes and is reported only as a secondary column.

**Run.** 3000 steps. Output: the final share of population A (the later one).

**Control.** The same checkpoint against itself, using disjoint samples. This gives the drift band.

**Data.** Runs l4Ln_s1 and l4Ln_s2: no locks, world growing from L52 by 8 every 500k steps, neuron limit growing
from 84 by 16. Checkpoints every 250k steps from 250k to 1.5M.

## 3. Results so far (l4Ln_s1; docs/results/tourney/)

| A vs B | Seeds | Share of A (neutral marker), mean [min, max] |
|---|---|---|
| 250k vs 250k (control) | 1 | 0.565 |
| 1.5M vs 1.5M (control) | 2 | 0.583 [0.418, 0.749] |
| 500k vs 250k | 2 | 0.732 [0.620, 0.844] |
| 750k vs 500k | 2 | 0.749 [0.673, 0.825] |
| 1.0M vs 750k | 2 | 0.637 [0.488, 0.787] |
| 1.25M vs 1.0M | 2 | 0.384 [0.273, 0.496] |
| 1.5M vs 1.25M | 2 | 0.533 [0.436, 0.630] |
| 1.5M vs 250k | 2 | 0.989 [0.978, 1.000] |
| 1.5M vs 250k, neuron limit 84 (250k era) | 1 | 0.981 |

Interpretation, interim:
- **The drift band is wide.** With 2 seeds the controls cover 0.42–0.75. Most neighbouring-era results fall inside
  it, so we cannot yet tell whether each 250k step is an improvement, or whether eras cycle. An earlier reading of
  "cycling between 1.0M and 1.25M" is withdrawn: it is within drift.
- **Long-range progress is clear.** 1.25M steps apart, the later population takes over (0.98–1.00).
- **It is not just bigger brains.** Under the old 84-neuron limit the 1.5M genomes still win 0.98.

## 4. Pending

- Seed 3, all pairs for l4Ln_s2, and the remaining n84 runs: `tools/tourney/run.sh` is running, and skips finished
  files.
- After a container restart, rerun:
  `nohup tools/tourney/run.sh build/rel/tourney docs/results/tourney runs6/l4Ln_s1 runs6/l4Ln_s2 &`
- Power: adjacent-era claims need at least 5 seeds each, or longer runs and a larger arena (lower drift).
- Summarise with `tools/tourney/analyse.py docs/results/tourney`.
