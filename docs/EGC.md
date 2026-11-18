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
