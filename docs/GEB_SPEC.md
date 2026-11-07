# Geb v08b3: Specification of the Reference Configuration

Derived from the source in `third_party/geb_v08b3/` (the oracle), cross-referenced to
Channon, *Unbounded evolutionary dynamics in a system of agents that actively process
and transform their environment*, GPEM 2006 (cited as GPEM §n). Where the source and the
paper differ, the source defines reference behaviour and the difference is listed in §10.

Only the compiled configuration is specified: the set of `#define`s active in the
shipped files with the makefile flags (`-DgebLinux`, and one of
`reset_shadow_to_real_for_each_pass` / `dont_reset_shadow_to_real_for_each_pass`).
Disabled branches (`test__use_four_inputs`, contexts, `protect_one`,
`clone__reproduce_over`, `ShadowLengthsFollowReals`, `compute_SHADOW_used_component_Complexities`)
are out of scope.

## 1. Random number generator (`rnd_core.cpp`)
- Generator: Numerical Recipes `ran1` (three LCGs with a 97-entry shuffle table),
  as adapted by Dave Cliff (1991). State: `ix1, ix2, ix3` (int), `r[98]` (float, index
  1..97 used), `iff`, `iset`, `gset` (Gaussian state; unused by Geb).
- Constants: M1=259200, IA1=7141, IC1=54773; M2=134456, IA2=8121, IC2=28411;
  M3=243000, IA3=4561, IC3=51349; RM1=1/M1, RM2=1/M2 (double).
- Table entries are `float((ix1 + ix2*RM2)*RM1)`; the return value is a float.
- Seeding: `randomize(seed)`; seed 0 means seed from the clock (`time % 32767`).
  Internally `ran1(&(-seed))` initialises and discards one value.
- `randval(L) = L * double(ran1())`; `irand(L) = floor(randval(L))`, redrawn while it
  equals L; `random(M) = M==0 ? 0 : irand(M)`. All Geb randomness is `random(M)`.
- Reproducibility requires IEEE single/double semantics with no excess precision
  (the original uses `-ffloat-store`; on x86-64 SSE this is the default) and no
  fast-math.

## 2. Genes and gene strings (`ann_core`)
- A gene is a bit (stored as `char` 0/1). A genotype is a non-empty gene array.
- A *gene string* is a span `[first,last]` into a gene buffer; `length = last-first+1`,
  or **-1 if the span is null**. Characters (node labels) are stored in *paired* form:
  `0,b1,0,b2,...`; information bits sit at odd offsets. `is_start_of(x)`: false if
  `length > x.length`; true if null; else element-wise prefix equality.
- `create_as_character("bits")` builds the paired form of an ASCII bit string.
- *Segment* (`get_segment(from)`): starting at `from`, advance by 2 while the current
  gene is 0 and the cursor is before the genotype's last gene; the segment ends one
  before the stop position. A segment may be empty (`last = first-1`, length 0).

## 3. Developmental system
### 3.1 Rule reading (`try_to_read_rule`)
At a position whose first two genes are `1,1`: predecessor = segment from +2;
replacement successor `Sr` = next segment (made null if length 0); new successor `Sn` =
next segment (null if length 0); then six link bits `b1..b6`, which must all lie within
the genotype (`from+5 <= last`), else no rule. `gene_string` spans predecessor start to
`b6`. b1/b2: reverse input/output link types on Sn; b3/b4: inhibitory/excitatory link
Sr->Sn; b5/b6: inhibitory/excitatory link Sn->Sr.

### 3.2 Rule generation and filtering (`generate_rules`, max-optimised variant)
1. Scan every position `g` with `last-g > 9`; if `genes[g]==genes[g+1]==1`, take the
   predecessor segment at `g+2`; record it as a *possible predecessor* if
   `pred.last + 9 <= last`. Order: genotype order.
2. Index possible predecessors by predecessor length (within a length, genotype order).
3. For each axiom node in node-list order, search lengths from
   `min(char.length, maxPredLen)` down to 0, within a length in order, for the first
   predecessor that is a prefix of the node character and whose rule (read lazily with
   `try_to_read_rule`) does not overlap (in genotype positions, inclusive) any
   already-kept rule other than itself. If found and not yet kept: keep it, insert into
   the kept list ordered by predecessor start.
4. Fixpoint: repeatedly walk the kept list (including entries inserted during the
   walk); for each kept rule not yet checked, for `Sr` then `Sn`, find the best
   non-overlapping matching rule as in step 3 and keep it if new. Stop when an
   iteration keeps nothing.
5. The organism's rule list = kept rules in *possible-predecessor (genotype) order*.
   These are the components recorded by rulestat.

### 3.3 Axiom network
Nodes in order: input `"001"`, hidden `"000"`, output `"01"`; excitatory links
input->hidden, hidden->output. The input node is a network input, the output node a
network output. `num_nodes = 3`. Development steps remaining: 99.

### 3.4 Development step (`morphogenesis_step`), once per organism update
Clear `just_constructed` on all nodes. Walk the node list (next pointer captured
before applying the rule). For each node not just constructed: pick the rule (first rule
in list order with the longest predecessor that is a prefix of the node character),
apply it, and stop the walk if `num_nodes == max_num_nodes` (default 20). Afterwards,
steps remaining = 0 if at the node limit, else decremented.

Applying rule R to node N (`run_rule`):
- If `Sn`: insert new node M directly after N with character `Sn`. Unless `Sr` exists and
  (b3 or b4), M copies N's input links (types swapped if b1). Unless `Sr` exists and
  (b5 or b6), M copies N's output links (types swapped if b2).
- If `Sr`: N's character becomes `Sr`; if `Sn`, add links per b3..b6.
  Else: remove N from the network input/output lists and delete N (and its links);
  `num_nodes--`.
- If `Sn` and N was a network input (output), M is appended as a network input (output).
- Mark R as used.
Links are a multiset: duplicate links count separately in excitatory sums.

## 4. Neurons (`calc_new_outputs`, `update_outputs`)
Synchronous update over the node list: compute all new outputs, then commit. For node N:
- inhibited = any inhibitory input source has inhibitory output 1.
- e = sum of excitatory outputs of excitatory input sources (+ network input value if N
  is a network input).
- new inhibitory output = (e >= t_v) with t_v = 500.
- If inhibited: new excitatory output = 0. Else: if `random(2)` then `e += random(600)`
  else `e = max(0, e - random(600))`; output = clamp(e >> 1, 0, 1024).
- Commit; network output entries take the node's excitatory output.
RNG is consumed per non-inhibited node in node-list order.

## 5. World and agents (`geb_core`)
- Toroidal grid `L x L` (default L=20); positions in 1/64 units; one agent per cell.
- `cosdeg64[t] = int(64 cos(0.017453293 t))`, `sindeg64` likewise.
- Agent birth: `direction_deg = random(360)`; set network inputs (§5.1); update outputs
  twice (priming level 2). Development is during life (one step per update).
- Initialisation: 50 agents with genotype `0` in cells 0..49 (row-major), each centred
  (+32); matching shadow genotypes `0`. `GA_time = -1`, then the first cycle starts
  (`GA_time = 0`). Minimum population 20, maximum `L*L`, no age limit.

### 5.1 Network inputs (`copy_net_inputs_from_organism_in_front`)
Zero all network-input values. For every other agent in the `(2c+1)^2` neighbourhood
(c=3, reduced to `(L-1)/2` if needed): torus-wrapped delta (dx,dy) in 1/64 units;
`d = long(sqrt(dx^2+dy^2))`; `scale = (256<<6)/d`; `right = (dx cos + dy sin) > 0`.
Each network input node whose first information bit equals `right` matches each of the
other agent's network output nodes whose character starts with the input character
minus its first pair; add `(scale * io) >> 6`.

### 5.2 Actions (`actions`), in this order
For each action, output = sum of io values of network output nodes whose character
starts with the action's prefix, capped at 1024; **if no node matches, output =
`random(500)`**.
1. Reproduce (`01`): if output > 500, try to reproduce (§5.3).
2. Kill (`100`): if output > 500, kill the agent in the next cell ahead (unless the
   population is at the minimum).
3. Turn anticlockwise (`101`) by `(60*o)>>10` degrees; 4. turn clockwise (`110`) likewise.
5. Move forward (`111`) by `(o<<6)>>10` units if the target cell is the same or empty.
`pen_colour` records the last of reproduce (1) / kill (2) attempted; default -1.
"Ahead" (`forward1`) steps by (sin, -cos) in 1/64 units until the cell changes.

### 5.3 Reproduction
Other parent = agent in the cell ahead (none: no-op). Child cell = cell ahead of the
other parent in this agent's direction; if occupied (or population at maximum), the
child replaces the other parent, unless the other parent's `pen_colour == 2` (killing),
in which case no-op. `reproduce()`:
- parent order random (`random(2)`); two distinct shadow parents chosen uniformly by
  index (redraw while identical pointer).
- crossover point p1 = `random(len1+1)`; p2 = p1 then `±1` (`random(100)<100` always,
  then `random(2)`); shadow likewise; clamp p2 to `[0, len2-1]`. Child = parent1[0,p1) +
  parent2[p2,len2). One bit-flip mutation at `random(childLen)`. Real then shadow.
- Child agent constructed (§5), `num_reproductions++`.
- If replacing: delete other parent (deletion removes a uniformly random shadow, §6).
- Append the shadow child (age 0, birth time now).

### 5.4 Main loop (`ga_main_loop`)
Each call processes one randomly chosen not-yet-processed slot (`random(remaining)`).
An agent is processed only if it is the same agent that occupied that cell at cycle
start and was not born this cycle: set inputs, one development step, update outputs,
actions, `age++`. When all slots are consumed: increment shadow ages for shadows not
born this cycle; start the next cycle (`GA_time++`, re-list occupied cells in cell order).
Agent identity: the original compares pointers; because newborns are excluded by birth
time, object identity is equivalent.

## 6. Shadow population
Shadow genotypes only (rules generated on demand). Whenever a real agent is deleted
(killed or replaced), a uniformly random shadow is deleted (swap-with-last). Whenever a
real child is born, a shadow child is bred from two random shadows with the same
operators. Variants:
- `no_resets`: the shadow evolves independently from initialisation.
- `resets`: after every rulestat snapshot, shadow `k` is set to a copy of the `k`-th real
  agent in cell order (with its age and birth time) (GPEM §8).

## 7. Rulestat output (`geb_rs`), every 1000 steps
Components are rule gene strings, bucketed by length; each bucket is an append-only list
with 1-based indices; a component's name is `length.index`. Per snapshot: real agents in
cell order, each rule in list order, abundance counted; complexity stats printed; line
`T: name abundance; ...` written to `rulestat` (non-zero abundances, bucket then index
order) with a verifier line. Then the shadow.
- `no_resets`: separate shadow buckets; same format to `SHADOWrulestat`.
- `resets`: shadow abundances are counted against the real buckets; shadow components
  not in the real list are appended after a `shadow_components_from` marker, named
  `length.index s<T/1000>` (index restarting at 1 for shadow-only entries), and stripped
  at the start of the next snapshot. Then shadows are reset to the reals.
- Quirk (both variants): when a bucket array grows, `shadow_components_from` markers are
  not copied (reset to 0).

## 8. Driver output (`geb_xwin.cpp`, headless)
- stderr: banner, "Could not open display ", "Running without a display", and on
  termination "\nProcess Terminated\n"; exit code 100 at `-stopat`.
- stdout every 100 steps with `-stats`: the three `time:` lines (integer-division means);
  exit 108 if population < 2. Every 1000 steps with `-rulestat`: the four `Complexities`
  lines.
- Options in scope: `-seed`, `-length`, `-neurons`, `-stats`, `-rulestat`, `-stopat`.

## 9. Not yet in the port
Snapshot save/restore (`geb_file`), X11 display, autofile/defragmentation restarts.

## 10. Differences between source and paper
1. **Component class.** GPEM §5 uses "predecessor plus link details" (P, b1..b6). The
   source records the whole rule gene string (P, Sr, Sn, b1..b6). The port records the
   source's class in `rulestat` and additionally emits a component dictionary so the
   paper's grouping can be applied in analysis.
2. **Initial population.** GPEM §3: every square starts occupied. Source: 50 agents.
3. **Activity statistics** are not computed by Geb; `rulestat` records abundances for an
   external tool (actiview, not distributed). The OEE-one analyser implements GPEM §2
   and §9 (isolated-occurrence screening, component activity normalisation, a0 from the
   most negative normalised activity).
