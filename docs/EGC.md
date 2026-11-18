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

- 7/7 transitioned (plain BFF: 3/7 on these seeds: 0, 2, 8; seed 8 collapsed there too), but 1/7 held.
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
| 8 | none (peak H 0.41) | 3,585, collapsed |

- Transitions: 4/8 in arm 10 against 4/8 in plain BFF; held to the end: arm 10 2/8, plain BFF 3/8.
- Table mutation changes the random trajectory from the first epochs on, so seeds are not paired between
  arms. Only rates over many seeds can be compared, and six seeds cannot separate these rates.
