# Evolvable genetic code (roadmap candidate 1, step 1)

Goal: let lineages define the meaning of their own code, so that heredity can change what programs mean,
not only what they contain (M5 found faithful BFF copying but no heritable variation or growth).

arXiv check: interpreters are fixed in almost all ALife systems and whether they can evolve is an open
question (arXiv 2603.08463); evolvable languages need mutation-robust encodings and a translation layer
like the biological genetic code (1105.1534, 1108.1589; Ofria, Adami and Collier 2002 via 1309.0719).

## Step 1: heritable codon table on the BFF soup (`oee_bff -egc 1|2`)

- Each 64-byte program holds a codon table in its first 10 bytes: byte value `tape[k]` means instruction
  k (order `[ ] + - . , < > { }`); every other value is a no-op.
- The table region is data, never executed (like tRNA). Code at position p is decoded with the table of
  the program that p belongs to, read live: running code can rewrite tables, and a copy that takes the
  code without the table changes what the copy means.
- Mode 1 (control): identical layout (54 executable bytes per program), table bytes inert, standard
  fixed code. Mode 2: evolvable code. The two differ only in whether the table is used.
- Mutation, pairing, epochs and the transition criterion are the paper's (131072 programs, mutation
  2^-12 per byte, transition when high-order entropy reaches 1).
- Checks (`tests/bff/test_egc.cpp`): with the standard code in both tables, mode 2 equals mode 1 on all
  10514 of 20000 random tapes whose tables were not rewritten; a program in a non-standard code runs
  under its own table and is inert under the standard one; checkpoints keep the mode. cubff equivalence
  of the plain mode is unchanged.
- Pilot (16384 programs, 1024 epochs, seed 3): executed instructions per interaction 49 (mode 1) vs 224
  (mode 2), high-order entropy 0.12 vs 0.37; mode 2 runs about 2.5x slower.

Questions:
1. Do self-replicators still emerge with an evolvable code, and how often relative to the control?
2. Do replicators carry their table (code co-copied with meaning), and do several codes ("dialects")
   coexist, or does one code take over?
3. Is post-transition diversity (distinct programs, high-order entropy) higher with an evolvable code?

Measures: transition epoch and rate (as M3), self-replicator share, and `tools/bff/egc_codes.py` on
checkpoints (distinct tables, top-table share, entropy of the table distribution, standard-code share,
dialects held by >= 1% of the soup).

Runs: `runs7/egc{1,2}_s{seed}`, launched by `tools/bff/egc_sweep.sh` lanes (seeds 0-4 in lane 1;
lane 2 with seeds 5-9 once the v9 runs free cores); restarted by `tools/ensure_supervisors.sh`.

## Interim (seeds 0-1)

| seed | fixed code (mode 1) | evolvable code (mode 2) |
|---|---|---|
| 0 | transition at epoch 10881 | no transition by 16384; H 0.23 |
| 1 | no transition (H 0.05) | no transition; H 0.22 |

In both evolvable soups the executed instructions per interaction stay high (about 230-245) while
high-order entropy falls below its random start (0.35 -> 0.22) and no codon table spreads (130,913 of
131,072 distinct at the end of seed 0). Two misses are within chance (about half of fixed-code soups
transition in this window), but consistent with replication being harder when a copy must also carry
its 10-byte table to keep its meaning.

Third arm added (mode 3 = `-egc 2 -egc-std`): evolvable code, every program starting with the standard
code. It asks the dialect question directly: once replicators exist in a shared language, do lineages
drift into new codes, and do several coexist? Lane order per seed is now mode 1, 2, 3.

## Table guard (after the standard-start arm)

With every program starting at the standard code (arm 3, no guard), the standard code was gone by epoch
~3,100 (share 0.0; 130,882 of 131,072 tables distinct). Cause: both heads start at position 0, A's table,
so random '+', '-' and copy instructions rewrite tables from the first steps; in the fixed-code control
those writes are harmless, here they scramble meaning. Small test (16384 programs, standard start, 512
epochs), share still holding the standard code: no guard 2.1%; heads start after the table 5.0%; '+'/'-'
cannot change tables 2.9%; both 5.0% (mutation alone would leave about 29%). Random copies remain the
main scrambler.

Literature (arXiv 0807.4749, 1110.5123, 0901.0181, 2510.10871): the biological code is a frozen accident
held by the cost of change, altered rarely and one codon at a time (codon capture, ambiguous
intermediate), and sits near an error-minimising optimum. Mapped back: tables should change only by
heredity (copying) and mutation, not by arbitrary arithmetic.

Guard 3 (`-egc-guard 3`): heads start after A's table and '+'/'-' cannot change table bytes; copies still
can, so replicators can pass their table on. The guard is stored in checkpoints.
Arms per seed from now: 1 fixed code; 2 evolvable, random tables, no guard (seeds 0-1 done);
4 evolvable, random tables, guard 3; 5 evolvable, standard start, guard 3. Arm 3 is kept for seed 0 only.

## Seed 0, all arms

| arm | transition | high-order entropy before/without transition | executed ops per interaction |
|---|---|---|---|
| 1 fixed code | epoch 10881 | ~0.05 | 30-50 |
| 2 evolvable, random tables, no guard | none by 16384 | ~0.23 | ~235 |
| 3 evolvable, standard start, no guard | none | ~0.24 (standard code lost by ~3,100) | ~250 |
| 4 evolvable, random tables, guard 3 | none | ~0.01 | ~140 |

Correction to the interim reading: the ~0.23 of the unguarded evolvable soups is not early structure from
replication but compressible byte patterns produced by random writes into tables; with the guard the soup
sits near the fixed-code control's pre-transition level (0.01 vs 0.05). Arm 5 (standard start, guard 3) next.

## Guard 7: aligned copying

At full size with guard 3 the standard code was gone by epoch 4,353 (131,066 of 131,072 tables distinct):
random copy instructions alone dissolve the code long before replicators appear (~10k epochs in the
control). Guard bit 4: a copy may write a table byte only from the same table position of a program
(aligned copying, which a replicator copying its whole tape does; random copies almost never align).
Small test (standard start, 512 epochs): guard 3 5.0% still standard, guard 7 28.5% (7,821 of 16,384
distinct) - the mutation-only expectation is ~29%, so random scrambling is removed and the code changes
only by mutation and faithful copying. Arms from now: 6 (random tables, guard 7) and 7 (standard start,
guard 7). The guard-3 standard-start run for seed 0 was stopped at epoch 4,353 (code already lost).

## Guard 15: slow table mutation (error threshold)

Seed 0, arm 6 (random tables, guard 7): no transition, H ~0.04. Arm 7 (standard start, guard 7): standard code
gone by epoch ~8,800 (131,071 of 131,072 distinct). With random scrambling removed, background mutation
alone (10 table bytes x 2^-12 per epoch, about 0.24% of tables per epoch) erases a shared code over
thousands of epochs, before replicators appear (~10k epochs in the control): an error-threshold problem;
the biological code mutates far more slowly than the genes it reads. Guard bit 8: table bytes mutate at
1/256 of the code rate. Small test (standard start, 2048 epochs): guard 7 0.6% standard (16,190 of 16,384
distinct), guard 15 98.0% standard (302 distinct): a near-frozen shared code with rare variants.
Arms from now: 8 (random tables, guard 15), 9 (standard start, guard 15). Arm 7 seed 0 stopped at ~8,800.

Scoring note: with every program starting at the standard code, identical tables alone put high-order
entropy at ~0.9 (arm 9, epoch 513), so the H >= 1 transition stop is not valid for standard-start arms;
they now run the full 16,384 epochs and replication is judged by the self-replicator count
(`bff_inspect CK`, now egc-aware: decodes with the checkpoint's code mode and guard).

Seed 0 self-replicators at the end: arm 1 (fixed) 2,069 of 131,072 (1.6%; transition at 10,881 did not hold
the M3 10% criterion); arm 2 (evolvable, no guard) 0; arm 8 (evolvable, random tables, guard 15) 0.
Seed 0 arm 9 (standard start, guard 15), 16,384 epochs: code held (84.5% standard, 4,026 distinct tables,
one dominant code) but no self-replicators (0 of 131,072); the fixed-code control of the same seed and layout
transitioned at 10,881. One seed; seeds 1-4 run arms 1, 2, 8, 9.
Seed 1 arm 8 (random tables, guard 15): no transition (H 0.02). Arm 9 (standard start, guard 15): no
self-replicators; code held at 83.0% standard (4,297 distinct) and a second code reached >= 1% of the soup -
the first dialect, spread by aligned copying among non-replicating programs (the seed-1 fixed-code control
did not transition either).
Seed 2: control no transition; arm 2 none; arm 8 none (H 0.02); arm 9 none (84.5% standard, 4,015 distinct, 0 selfrep).
Seed 5 control (arm 1): transition at epoch 1,281; at 3,329, 127,783 of 131,072 programs are self-replicators
(97.5%, 8,992 distinct) - the strongest control so far, and the first seed where the evolvable arms face a
control that transitions early and holds.
Seed 5 arm 2 (evolvable, no guard): no transition in 16,384 epochs (H 0.25), against the same seed's control
transitioning at 1,281 to 97.5% self-replicators - the clearest contrast so far. Seed 3 control: no transition.
Lane 3 added (seeds 10-12) on the core freed by the v9 runs.

## Replicators first, then an evolvable code (option 1)

Seed 5's fixed-code soup at epoch 3,329 (127,783 of 131,072 self-replicators) was switched to the evolvable
code with the standard code written into every table (`-switch-egc M G`). 128 epochs later:
guard 14 (no +/- on tables, aligned copies, slow table mutation): 0 self-replicators, 99.9% still standard;
guard 8 (slow mutation only): 0, 5.8% standard; no guard: 0, 4.3% standard.
BFF replicators start their heads at position 0 - the table region - and use those bytes as working memory
(loop counters). Protecting the table breaks their loops; not protecting it lets them overwrite their own code
within ~100 epochs. In this soup any code stored in addressable memory collides with program memory, which
explains the whole sweep (no evolvable arm held replicators on any seed).
Next design (option 2): keep each program's codon table outside the tape, unreachable by instructions, and
let a program inherit the table of the program whose bytes it carries after an interaction (vertical
transmission of the translation machinery), with rare table mutation. Lanes 3-4 stopped; lanes 1-2 finish
seeds 0-9 of the original arms.

## Option 2: tables outside the tape (`oee_bff -egc 3|4`)

- Each program keeps a 10-byte table in a separate array; all 64 tape bytes are code and data, and no
  instruction can address a table.
- Each instruction is decoded with the table of the program it lies in.
- Authorship: every write records whether the code that wrote the byte runs in A or B. After the
  interaction, a program inherits the other's table if more than half of its bytes were last written by the
  other program. A replicator that overwrites its partner therefore hands over its code with its bytes.
- Table bytes mutate at `mutation >> tab_shift` (default 8) per byte per interaction, before execution.
- Mode 3 keeps every table standard and fixed. It reproduces plain BFF bit for bit (test bff_egc, and an
  8192-program soup log identical to cubff mode over 512 epochs), so the M3 seed sweep is its control.
- Mode 4 is the evolvable code: arm 10 starts from the standard code (`-egc-std`), arm 11 from random tables.
  Since the tables are not on the tape, they no longer inflate the high-order entropy, and the normal
  H >= 1 transition stop applies to both arms.
- Extra log columns: distinct tables, share of standard tables, share of the most common table, and the
  number of tables passed by authorship in the last epoch.
- Runs: lanes 3-4 (runs7/lane3, lane4; arms in laneN.arms), seeds 0 and 5 first, since the fixed-code
  control transitioned on both. Lanes 1-2 were stopped after their current runs (args kept as `.paused`):
  option 1's result is already clear from seeds 0-5.

### Interim: seed 5, random tables (arm 11)

- Transition at about epoch 4,400. One non-standard table, [110, 236, 36, 160, 41, 159, 195, 72, 104, 228],
  went from 0.4% of programs (epoch 3,969) to 95.6% (epoch 4,737). Tables passed by authorship rose from
  about 10 to about 140 per epoch.
- Inspect at epoch 4,608: 12,092 self-replicators (9.2%), scored under each program's own table.
- High-order entropy peaked at 1.52 and is falling (0.89 at 4,737). Collapse or hold is decided by the end
  of the run.
- First evolvable-code arm with replicators. Its code spread with its replicators, as option 2 intends.
- Plain BFF seed 5 (the mode-3 control, docs/results/m3/sweep.md) has no transition in 16,384 epochs. One
  seed, so no rate claim yet.

### Seeds 5 (arm 11) and 0 (arm 10), final

| run | code start | end epoch | self-replicators | standard tables | most common table | plain BFF, same seed |
|---|---|---|---|---|---|---|
| egc11_s5 | random | 6,273 (transition + 2048) | 129,415 (98.7%) | 0% | 99.8% (evolved) | no transition |
| egc10_s0 | standard | 16,384 | 0 | 87.5% | 87.5% (standard) | transition at 2,433, held |
| egc11_s0 | random | 10,241 (transition ~8,200 + 2048) | 0 | 0% | 48.5% (evolved) | transition at 2,433, held |
| egc11_s6 | random | 8,449 (transition ~6,400 + 2048) | 0 | 0% | 61.5% (evolved) | no transition |
| egc10_s1 | standard | 16,384 (limit) | 129,256 (98.6%) | 47.9% | 51.9% (evolved, see below) | no transition |
| egc10_s5 | standard | 2,753 (transition ~700 + 2048) | 147 (0.1%) | 98.7% | 98.7% (standard) | no transition |

- Seed 5, random codes: the soup held. High-order entropy reached 4.90, and a single evolved code carries
  nearly every program. This is the first soup in this project with replicators under a code that evolved
  rather than being given.
- Seed 0, standard start: the tapes are mutated exactly as in plain BFF, which transitioned at 2,433 on this
  seed. Here there was no transition: peak H 0.37 at epoch 833, and the share of standard tables fell to
  87.5%. A plausible cause is that mixed codes break the early proto-replicators: a copy loop only works if
  partner programs read the copied bytes with the same code. Not tested; one seed per arm.

- Seed 0 with random codes and seed 5 with the standard start both transitioned and then collapsed:
  - egc11_s0 peaked at H 1.66 and ended at 0.24;
  - egc10_s5 peaked at 3.24 and ended at 0.75.
  This matches the collapses in plain BFF seeds 8, 11 and 16. Running tally for option 2: 3 of 4 runs
  transitioned, and 1 of 4 held.

- egc11_s6 transitioned (peak H 1.56 at 6,465) and collapsed (0 self-replicators at the end).
- All 3 random-code arms so far transitioned, including seeds 5 and 6, where plain BFF does not. Only the
  run in which one code reached 99.8% held. In the two collapses the most common code covered only
  49-62% of programs.
- Working hypothesis, untested: coexisting codes make cross-code interactions copy garbage. A replicator
  lineage survives only if its code sweeps the soup before that happens.
- Test: share of the top code against replicator count over time in the held run and in the collapsed runs.
- Logs, epochs after H first passes 0.6:
  - held run (s5): the top code went from 30% to 95% within 256 epochs, and H then rose to 4.9;
  - collapses (s0, s6): the top code climbed slowly (17% to 42%, 31% to 52%) while H fell below 0.8.
  This is consistent with the race hypothesis: the code must sweep faster than the replicators decay.
  The cause of the decay is not yet measured.

### Seed 1, standard start: a codon reassignment invades a replicator soup

- Transition at about epoch 14,300. The replicators run on the standard code, which reached 99.98% of
  tables by 14,465. H was stable at about 2.5.
- At about 15,800 a variant table appeared that encodes `]` as byte 74 (`J`) instead of 93, with the
  other nine codons standard. It spread from 0 to 52% by 16,321, while H jumped a second time, from 2.5 to
  4.4.
- At the end (16,384 epochs): 68,036 variant tables and 62,761 standard ones, with 98.6% self-replicators.
- This is the first case here of a code change invading an established replicator population. Programs
  that use `J` for `]` are a separate lineage, and the second H jump suggests a new replicator type rather
  than a neutral relabelling. Not yet checked: whether the variant lineage's programs contain byte 74 where
  the standard lineage has 93.
- Continued from the final checkpoint to 32,768 epochs in runs7/egc10_s1x, keeping a checkpoint every 1,024
  epochs. This run is not managed by the lanes: after a container restart, resume it by hand:
  `runs7/bin/oee_bff_egc7 -epochs 32768 -interval 64 -log runs7/egc10_s1x/log.csv -save runs7/egc10_s1x/ck -save-every 256 -keep-every 1024 -resume runs7/egc10_s1x/ck`.
- Check of the variant lineage (final checkpoint of egc10_s1), mean bytes per program:

  | table | programs | byte 74 (`J`) | byte 93 (`]`) |
  |---|---|---|---|
  | standard | 62,761 | 2.32 | 1.95 |
  | variant | 68,036 | 3.19 | 0.03 |

  - In the variant lineage, byte 93 has almost disappeared (it is a no-op there), and `J` takes its place
    as loop end. So the reassignment is used, not neutral.
  - The standard lineage already carries about 2.3 `J` bytes per program as inert data. Reassigning `]` to
    an existing inert byte turns those bytes into loop ends at once, so this looks like codon capture.
  - In both lineages the most common program covers only 0.1-0.2%, so each lineage is diverse.

### Random-code tally (arm 11)

| seed | transition (peak H, epoch) | end H | top code at end | outcome |
|---|---|---|---|---|
| 0 | 1.66, 8,257 | 0.24 | 48.5% | collapsed (0 self-replicators) |
| 1 | 1.34, 10,241 | 0.30 | 58.7% | collapsed (by H; not inspected) |
| 5 | 4.90, 6,273 | 4.90 | 99.8% | held (98.7% self-replicators) |
| 6 | 1.56, 6,465 | 0.22 | 61.5% | collapsed (0 self-replicators) |
| 7 | 1.62, 5,313 | 0.32 | 57.8% | collapsed (by H; not inspected) |
| 2 | 1.56, 3,457 | 0.09 | 60.8% | collapsed (by H; not inspected) |
| 8 | 1.45, 1,281 | 0.27 | 55.2% | collapsed (by H; not inspected) |
| 9 | 1.54, 3,713 | 0.27 | 55.7% | collapsed (by H; not inspected) |
| 4 | 3.51, 9,537 | 3.51 | 96.3% | held (63.7% self-replicators, H still rising) |
| 15 | 1.89, 10,497 | 0.43 | 51.8% | collapsed (by H; not inspected) |
| 10 | 1.59, 4,545 | 0.35 | 57.3% | collapsed (by H; not inspected) |
| 11 | 1.79, 4,481 | 0.79 | 81.4% | declining (11.0% self-replicators at the end, 129,325 distinct) |
| 16 | 1.79, 4,033 | 0.40 | 58.2% | collapsed (by H; not inspected) |
| 12 | 1.27, 10,049 | 0.28 | 62.1% | collapsed (by H; not inspected) |
| 17 | 1.72, 8,385 | 0.14 | 47.7% | collapsed (by H; not inspected) |
| 18 | 4.80, 6,529 | 4.80 | 99.7% | held (98.2% self-replicators) |
| 19 | 1.61, 1,025 | 0.41 | 56.1% | collapsed (by H; not inspected) |
| 13 | 1.39, 1,025 | 0.19 | 52.4% | collapsed (by H; not inspected) |
| 3 | 2.59, 3,777 | 2.49 | 99.97% | not held: code swept, but only 4.4% self-replicators (5,776; 40,962 distinct) |
| 14 | 4.41, 6,273 | 4.41 | 99.97% | held (98.3% self-replicators); predicted from top share 99.8% at H 3.9 |

- 9/9 transitioned (plain BFF: 3/9 on these seeds: 0, 2, 8; seed 8 collapsed there too), but 2/9 held. Both
  runs that held are the two in which one code passed 95% of the soup.
- With random codes, transitions come easily but rarely last. The one that held is the one whose code
  swept the soup, which supports the race hypothesis above.
- The standard-start arm transitions less often (4/8) but held 2/4, including the codon-capture run.

### Standard-start tally (arm 10)

| seed | arm 10 | plain BFF (M3) |
|---|---|---|
| 0 | none (peak H 0.37) | 2,433, held |
| 1 | ~14,300, held (codon capture), continuing | none |
| 2 | none (peak H 0.36) | 11,585, held |
| 5 | ~700, collapsed | none |
| 6 | none | none |
| 7 | ~8,000, held (H 5.07 at 9,857, 92.6% self-replicators, standard code 99.4%) | 12,481, held |
| 3 | none (peak H 0.37) | none |
| 4 | none (peak H 0.39) | none |
| 9 | none (peak H 0.36) | none |
| 10 | none (peak H 0.39) | 14,081, held |
| 11 | ~1,600, held (H 4.81 at 3,649, 97.4% self-replicators) | 2,305, collapsed |
| 12 | ~14,300, held (H 4.18, 88.5% self-replicators, standard code) | 11,585, held |
| 15 | ~10,300, held (H 5.37, 96.1% self-replicators, variant code `-`=89) | 5,953, held |
| 16 | none (peak H 0.39) | 5,377, collapsed |
| 14 | none | none |
| 18 | none (peak H 0.37) | none |
| 19 | none | none |
| 13 | ~7,400, held (H 5.22, 98.5% self-replicators; `]`=142 variant 71.9%, standard 27.3%) | none |
| 8 | none (peak H 0.41) | 3,585, collapsed |

- Final, 20 seeds: transitions 8/20 in arm 10 against 9/20 in plain BFF; held to the end 6/20 in each.
  Three of the six arm-10 holds run on an evolved code (seeds 1, 13, 15). Random start (arm 11): 20/20
  transitioned, 4 held (seeds 4, 5, 14, 18).
- Seeds 10-19 of both arms started in lanes 3-4, which gives 20 seeds per arm to compare with the 20-seed M3
  sweep.
- Table mutation changes the random trajectory from the first epochs on, so seeds are not paired between
  arms. Only rates over many seeds can be compared, and six seeds cannot separate these rates.
- Continuation (runs7/egc10_s1x), epochs 16,384 to 25,409:
  - The `J` variant rose to 83% by 24,449, then settled at about 78%. The standard code holds 18-19%.
  - H rose from 4.4 to about 6.0 and plateaued.
  - From about epoch 24,000, second-step variants built on the `J` table persist at 1-3% of the soup:
    `,` on byte 24 (1,599 programs) and `-` on byte 157 (1,384 programs).
  - Each codon change so far has been a single reassignment on the currently dominant table, so the code
    evolves as a lineage.
- From epoch 27,000 the standard code is winning back ground. Its share went from 18% (27,009) to 37%
  (28,545), and the `J` share fell from 79% to 62%. H stayed at about 6.0, and the second-step variants
  fell back to about 1%.
- So the two codes are not on a one-way sweep, which fits two coexisting lineages under frequency-dependent
  selection. Two possible mechanisms:
  - each lineage may overwrite the other's programs, so that whoever authors the bytes passes on its
    table;
  - inert bytes may have different effects in each lineage, `]` in the `J` lineage and `J` in the standard
    one.
  The kept checkpoints (every 1,024 epochs) allow per-lineage replicator counts and cross-lineage overwrite
  rates. That analysis is the next step once the run ends at 32,768.

### Seed 1 lineages over time (`tools/bff/egc_lineages.py`, 512 programs sampled per table)

| epoch | `J` lineage | standard | self-replicators, `J` / standard |
|---|---|---|---|
| 17,408 | 59.8% | 39.8% | 98% / 98% |
| 20,480 | 70.1% | 29.7% | 97% / 97% |
| 23,552 | 82.9% | 16.8% | 95% / 93% |
| 25,600 | 79.2% | 18.2% | 87% / 88% |
| 28,672 | 63.0% | 35.9% | 86% / 83% |
| 30,720 | 79.1% | 19.6% | 89% / 88% |
| 32,768 | 77.6% | 21.3% | 92% / 92% |

- The two lineages have the same self-replicator share at every checkpoint. Neither code makes better
  replicators.
- The `J` share swings by 15-20 points within 1,000-2,000 epochs. In a population of 131,072 this is far too
  fast for neutral drift of the tables alone. Because tables travel with the bytes, they most likely
  hitchhike on program-level sweeps: whichever programs win the copying competition carry their code with
  them. Not yet tested: the dominant program family per lineage around a swing.
- Third tables come and go at about 1%, on both backgrounds (`J`+`,`=24, standard+`+`=98, standard+`<`=216),
  all with the same replicator share.
- Reading: in this soup the code is an evolvable, heritable trait that changes and diversifies, but it is
  under weak or no selection. It mostly marks lineages. Getting selection on the code itself would need
  codes that differ in function, for example a cost per distinct instruction, or new instructions available
  only to some codes.

### Seed 15, standard start: replicators arise on a variant code

- At epoch 10,241 the standard code covered 97.3% of tables. By 10,625 it covered 0%, and a variant with
  `-` encoded as byte 89 (`Y`) instead of 45 covered 92.9%. At the same time H jumped from 1.23 to 3.59, so
  the replicators that took over carried the variant code. The code changed as part of the transition, not
  after it.
- End (13,825, transition + 2048): H 5.37, variant 92.2%. A second reassignment of `-`, to byte 240, holds
  6.6%. `+`=71 on the `Y` background holds 1.0%.
- Plain BFF seed 15 transitioned at 5,953 and held.
- Second seed in arm 10 whose dominant replicators use a non-standard code, after seed 1. Both cases are
  single reassignments of one instruction.

### Seed 13, standard start: a second `]` reassignment (interim)

- The transition happened on the standard code: at epoch 7,553 the standard code covered 100% of tables
  and H was 3.23.
- By 7,937 a variant encoding `]` as byte 142 already held 42.1% (55,119 tables). H jumped a second time,
  from 3.27 to 4.23, the same pattern as seed 1 (there `]` became byte 74).
- This is the third arm-10 soup with a codon change in a replicator population (seeds 1, 13, 15). Two of
  the three reassign `]`, the loop end.
- A possible reason, untested: `]` is the only instruction whose meaning depends on matching. Moving it
  changes which loops close, so the variant can rewrite loop structure in one step.

- Prediction test (seed 18, random codes): one code was already at 99.3% when the transition started, so the
  race hypothesis predicted a hold. It held (98.2% self-replicators, H 4.80). Random-code tally: 16/16
  transitioned and 3/16 held. All 3 that held have a top code above 95%. All 13 that did not stayed between
  48% and 81%.
- Seed 14 (random codes) was predicted to hold from its top share (99.8% at H 3.9), and it held (98.3%
  self-replicators). Random-code tally 17/17 transitioned, 4/17 held. All 4 holds have top share > 95%; none
  of the 13 non-holds reached 82%. Plain BFF on these 17 seeds held 6 times, so random codes hold less
  often, but every hold sits on an evolved, non-standard code.
- Seed 19 (random codes): transitioned and collapsed, top share 56.1%. Random-code tally: 18/18 transitioned,
  4/18 held. The rule still separates them.

## Next design: a code that can be selected (option 3, planned)

Problem: in option 2 every table maps exactly one byte to each instruction, so all codes are functionally
equivalent. A reassignment only relabels bytes, and selection cannot act on the code itself (seed 1
lineages, above).

Prior work:
- Interplay of Fidelity and Diversity in the Evolution of the Genetic Code (arXiv 2510.10871, 2025). Code
  optimality is a trade-off between translation-error minimisation (fidelity) and matching the composition
  of what is encoded (diversity).
- Neutral emergence of error minimisation through code expansion (PMC4500140). Robust codes can arise from
  codon duplication without direct selection.
- Computational Life (2406.19108): the BFF base.
- Avida instruction-set studies (langev.com/pdf/ofria02ieee.pdf): instruction sets differ in evolvability,
  so the encoding matters.

Mapping to BFF: replace the 10-byte table with a 256-entry translation map, one entry per byte value (an
instruction or a no-op). A mutation changes one entry.
- An instruction may then be encoded by several bytes (synonymous codons). Byte mutations between synonyms
  are neutral, so fidelity, that is robustness to the 1/2^12 per-byte mutation, becomes selectable.
- More no-op entries give more bytes free for data, which is the diversity side.
- Inheritance by authorship is unchanged.
- Measures:
  - degeneracy per instruction over time;
  - the robustness of the dominant replicator to point mutations under its own code against under
    shuffled codes;
  - whether lineages with more synonyms displace others. Selection on the code would show as a sweep that
    is not explained by hitchhiking.
- Control: the same map but frozen at the standard code (equivalent to plain BFF).
- Built (`-egc 5`, binary runs7/bin/oee_bff_egc8). A mutated entry becomes an instruction with chance 10/256
  and a no-op otherwise, so the neutral expectation is about 10 coding entries per map. Mean coding entries
  above that would mean synonyms are being selected.
- Checks (test bff_egc):
  - a frozen standard map reproduces plain BFF over 20,000 random tapes;
  - an 8192-program soup log over 300 epochs is identical to plain BFF;
  - a synonym executes as its instruction;
  - the checkpoint keeps the maps.
- New log columns: `top_coding` (coding entries in the most common map) and `mean_coding`.
- Runs: lane 3, seeds 0-9, arm 12 (standard start) and arm 13 (random start).
- Correction (seeds 3 and 13 had been left out of the tally): random start, 20 seeds, 20/20 transitioned,
  4/20 held.
- Seed 3 is a counterexample to "code sweep implies hold". The top code reached 99.97%, and H stayed at
  2.5, but only 4.4% of programs were self-replicators at the end.
- Revised rule: a sweep above 95% is necessary for a hold (it held in all 4 cases, and none of the 15
  others without a sweep held) but not sufficient (seed 3).

### Option 3 interim: coding entries rise before any transition (seed 0, random maps)

| epoch | mean coding entries | coding entries in top map | top map share |
|---|---|---|---|
| 1 | 10.00 | - | 0.00% |
| 1,025 | 10.17 | 16 | 0.13% |
| 3,073 | 10.54 | 20 | 0.46% |
| 5,121 | 11.18 | 20 | 1.53% |

- The mean is above the neutral 10 and rises steadily. The commonest maps carry 16-20 coding entries. H is
  still 0.2-0.26, so there are no replicators yet.
- Seed 10 (random maps) stays flat at 10.2.
- A likely cause, untested, is authorship bias, not robustness. A map with more coding bytes makes its
  programs execute more instructions, so they write more bytes, author more partners and pass the map on.
  That is selection on the code, but for write activity. A check: correlate a map's coding count with the
  number of tables it passes on in each epoch.
