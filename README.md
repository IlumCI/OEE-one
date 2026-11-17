# OEE-one

Open-ended evolution (OEE) as the route to general and, eventually, superintelligence. The system
stacks the mechanisms that produced verified open-ended results in artificial life (Geb, BFF
program soups, fraternal transitions) and measures every step against null controls, so that
"open-ended" is a tested claim rather than an impression.

- [docs/DESIGN.md](docs/DESIGN.md) - layers L0-L5, prior-system synthesis, measurement, containment
- [docs/PLAN.md](docs/PLAN.md) - milestones and ranked candidate directions
- [docs/COMPUTE.md](docs/COMPUTE.md) - engine, parallelism, storage
- [docs/GEB_SPEC.md](docs/GEB_SPEC.md) - Geb v08b3 as ported

## Status

| Layer / milestone | Result | Details |
|---|---|---|
| M1, L2 organisms | C++20 port of Geb v08b3, byte-identical to the original; Channon's class 3b (unbounded activity) reproduced on 5 seeds to 8M steps, null controls flat; frozen runs rescued by rollback and reseed; Channon 2019 joint-scaling result reproduced qualitatively | [RESULTS](docs/RESULTS.md), [SCALING](docs/SCALING.md) |
| M2, measurement | Multi-scale path divergence (arXiv 2606.17091) and a calibrated real-vs-null test | [M2](docs/M2.md) |
| M3, L1 chemistry | BFF soup reproduced (20 seeds, 2D spatial version, replicator fronts) | [M3](docs/M3.md) |
| M5, L1 in L2 | BFF code as Geb heredity: faithful copying, no variation or growth; paused | [M5](docs/M5.md) |
| L3 transitions | Kin colonies (v4), colony reproduction (v5), division of labour (v6), growing world (v7), world and brain growing together (v8), earned growth with a neuron cost (v9) | [L3](docs/L3.md) |

Main findings so far:
- Diversity is capacity-limited (about 0.3-0.36 components per cell); a growing world gives
  unbounded diversity, but individual complexity also needs per-agent capacity to grow (v7 vs v8).
- Collectives change the shape of growth: under the same joint growth, plain Geb plateaus at about
  14.8 while colony worlds keep rising and hold 3-5x the diversity (v8 vs its control, 3 vs 4 seeds).
- v9 (3 seeds complete, 2 running): space earned by collective reproduction brings the complexity
  rise forward; a yoked control that receives the same space at the same times ends higher.

Not built yet: L0 physics, L4 co-evolved challenges, L5 self-modification, the outer meta-search.

## Layout

```
src/geb/        Geb world, agents, development, networks, activity bookkeeping (C++20)
src/bff/        BFF program soup
apps/           oee_geb (simulator), oee_bff
tools/activity  Bedau-Packard activity, component normalisation, D series, OEE classification
tools/l3        group and complexity series
tools/storage   pack / archive / restore run data
tools/showcase  generative-art renderer for site/
tests/          golden equivalence, resume, rescue, activity and MSPD checks
third_party/    original Geb v08b3 (MIT, unmodified) used as the reference oracle
docs/results/   result tables per milestone
site/           showcase page and media
```

## Build and test

```
cmake --preset release && cmake --build --preset release && ctest --preset release
cmake --preset asan    && cmake --build --preset asan    && ctest --preset asan
tools/geb_reference/build.sh            # optional: build the original as an oracle
```

## Run

```
build/rel/oee_geb_resets -seed 1 -stats -rulestat -log log.txt -second-shadow -presence presence \
    -save ck -save-every 20000 -stopat 3000000
```

| Option | Meaning |
|---|---|
| `-seed -length -neurons -stats -rulestat -stopat` | original Geb options |
| `-second-shadow`, `-presence FILE`, `-components FILE` | independent null shadow and component records for the activity tests |
| `-random-selection` | GebR negative control |
| `-collectives`, `-collectives-random`, `-senescence`, `-colony-reproduction`, `-division-of-labour` | L3 rules v4-v6 and their controls |
| `-grow-every -grow-by -grow-max -grow-neurons-by -grow-neurons-max` | scheduled growth of world and neuron limit (v7, v8) |
| `-earned-growth`, `-neuron-cost`, `-grow-follow LOG` | v9: growth bought by collective births, per-neuron death hazard, yoked control |
| `-crng` | per-agent counter RNG streams (also switches a resumed legacy run) |
| `-threads N` | order-equivalent parallel engine, byte-identical to 1 thread (counter-RNG mode) |
| `-save -save-every -archive-every -archive-keep-log -resume -reseed -freeze-exit` | checkpoints, retention, resume, rescue |
| `-heredity bff\|bff0\|bffv`, `-founders` | M5 BFF heredity |

Long runs are kept alive by `tools/supervise_runs.sh <runs_dir> <bin_dir>`, which reads
`<runs_dir>/runs.txt` (`name variant seed stopat args...`), resumes from checkpoints, rescues frozen
runs, supports several hosts (`@host` tags, leases), and archives finished runs automatically.
`tools/ensure_supervisors.sh` restarts supervisors after a machine restart.

## Analysis

```
python3 tools/activity/geb_activity.py RUN --variant resets --grouping pb --source presence [--null] --plot
python3 tools/activity/d_series.py RUN [RUN ...] [--grouping rule|pb] [--window 500000]
python3 tools/l3/complexity_series.py RUN [RUN ...] [--window 500000]
python3 tools/activity/report.py OUT RUN [RUN ...]
```

## Run data

Raw run directories are not on the main branch. Finished runs are archived to the `run-data`
branch as xz-compressed, checksummed parts; locally each becomes a stub with an `ARCHIVED` file.

```
tools/storage/restore_run.sh runs6/l3v9_s1     # fetch only that run's parts, verify, unpack
tools/storage/archive_runs.sh TMP 400 RUNDIR...  # manual archive (KEEP=1: copy only; ANY=1: no 'finished' needed)
```
