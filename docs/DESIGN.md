# OEE-one: Founding Design Document

## 1. Objective

Reach open-ended evolution (OEE) as the route to artificial superintelligence (ASI).
The system combines the mechanisms that produced verified OEE results in prior
artificial life systems, adds the mechanisms needed to convert open-ended complexity
into general intelligence, and treats undecidability and computational irreducibility
as features to exploit rather than limits to avoid.

### 1.1 Position on transformers

Next-token prediction optimises imitation of a fixed distribution and contains no
objective term for producing novelty outside it. It is therefore not treated as a path
to open-endedness. Transformers are used only as temporary scaffolding under the policy
in Section 4, and are purged when they hinder progress or become resource-expensive.

Note: the halting problem applies to every Turing-equivalent system, including this one.
Undecidability is argued to be a necessary condition for OEE (Hernandez-Orozco et al.,
arXiv:1606.01810). The design goal is a system that continually runs into computations
whose outcome cannot be predicted, rather than one that converges on reproducing its
training data.

## 2. Prior systems and what is taken from each

| Source | Verified result | Component taken |
|---|---|---|
| Geb (Channon 2001, 2002, 2006, 2019) | Only system formally classified Class 3 (unbounded) under Bedau-Packard activity statistics, including the component-normalised revision. Maximum individual complexity indefinitely scalable when world size and per-agent capacity are scaled jointly; growth is logarithmic. | Endogenous fitness, environment-transforming agents, joint-scaling test, activity statistics. |
| Chromaria (Soros & Stanley 2014; Soros, Cheney & Stanley 2016) | Stagnation when any of four hypothesised conditions is removed; the strictness of the minimal criterion decides between stagnation and divergence. | Minimal-criterion reproduction gate, adaptive criterion strictness, no representational cap on complexity. |
| Hash Chemistry (Sayama 2018/19; Horiguchi & Sayama 2026) | Cumulative count of unique replicators grew roughly linearly with no visible bound; higher-order entities emerged; random-fitness control did not. Spatial version: space size controls the transition between a compact-replicator regime and a runaway size-dominance regime. | Fitness evaluation over entities of any size (cardinality leap). |
| Adams, Zenil, Davies & Walker (2017) | Formal criteria for unbounded evolution and innovation; only state-dependent rule updates produced OEE in a way that scales. | State-dependent physics: the world rewrites its rules based on its own state. |
| Flow-Lenia (Plantec et al. 2022, 2025) | Mass-conserving continuous CA with parameters stored in matter; multispecies dynamics. | Substrate physics, resource conservation, rule parameters stored in matter. |
| Computational Life / BFF (Aguera y Arcas et al. 2024) | Self-replicators emerge from random programs without any fitness function. | Self-modifying program soup as the chemistry layer. |
| DISHTINY (Moreno & Ofria 2019, 2022) | Fraternal transitions in individuality; evolved multicellular life histories. | Hierarchical individuality: individuals merge into higher-level individuals. |
| POET / Enhanced POET / OMNI-EPIC | Co-generation of environments and solutions. | Open-ended challenge generation (transformer parts scaffolded only). |
| Darwin Godel Machine (2025) | Archive-based open-ended self-modification of agent code. | Recursive self-modification (LLM mutator scaffolded only). |
| ASAL / ASAL++, multi-scale path divergence (2024-2026) | Automated search over substrate rules for open-ended regimes. | Meta-search over physics (VLM scoring scaffolded only). |
| MODES (Dolson et al. 2019) | Metrics for change, novelty, complexity, ecology. | Instrumentation. |

## 3. Architecture

### L0 Physics
Flow-Lenia-style substrate: mass conservation, rule parameters stored in matter and
evolving with it. Rule updates depend on the system's own state (Adams et al.). Built
for GPU/JAX from the start.

### L1 Chemistry
BFF-style self-modifying program soup in which replicators arise without a fitness
function. Hash Chemistry evaluator over compositions of any size, so composite entities
have no size ceiling.

### L2 Organisms
Geb-style agents: no explicit fitness function, active transformation of the
environment, genotypes uncapped by the representation. Reproduction gated by a minimal
criterion whose strictness adapts to avoid both too-strict and too-relaxed stagnation.

### L3 Major transitions
DISHTINY-style fraternal transitions. Rationale: Geb's maximum complexity grows
logarithmically with scaled resources, which is insufficient for ASI. Transitions in
individuality are the known mechanism that converts accumulated complexity into new
hierarchical levels.

### L4 Open-ended challenges
Co-evolved environments and tasks that supply pressure toward general problem-solving.
The pressure is applied by changing the environment, not by scoring agents directly, so
fitness stays endogenous.

### L5 Self-modification
Archive-based recursive self-modification: sufficiently capable lineages evolve their
own variation and selection machinery.

### Outer loop: meta-search
Search over L0 physics for worlds whose dynamics stay open-ended.

### Instrumentation (all layers)
- Bedau-Packard activity statistics with component normalisation
- Channon joint-scaling test
- MODES (change, novelty, complexity, ecology)
- Multi-scale path divergence

Primary success criterion: Class 3 under the component-normalised activity test, plus
complexity that keeps scaling when world size and per-agent capacity are scaled jointly.
Foundation-model interestingness scores are never a success criterion.

### Containment
Sandboxed compute. Capability tripwires driven by the instrumentation layer
(cf. arXiv:2502.04512).

## 4. Transformer scaffold policy

### 4.1 Boundary
- Transformers are never part of any genotype, phenotype or reproduction step. They sit
  outside the world and produce only data: parameter sets, environments, proposed
  mutations. They are never live components that lineages call.
- Every transformer-produced artifact carries a provenance tag that passes down to
  descendants.

### 4.2 Front-loading (phase 1)
Use transformers heavily early. Each use is paired with a native replacement that is
trained or evolved to make it unnecessary.

| Transformer use | Native replacement being distilled |
|---|---|
| VLM-scored search over physics parameters (ASAL) | Path divergence and activity statistics, calibrated against ASAL scores on the same runs |
| Environment generation (OMNI-EPIC), run offline into an archive | Co-evolved environment generators seeded from the archive |
| LLM-proposed mutations (DGM) | Evolvable mutation operators encoded in the substrate |

Phase 1 exists to transfer capability into the substrate, not to make progress that
depends on transformers.

### 4.3 Purge triggers (automatic; defaults to tune)
1. Resources: transformer compute exceeds 20% of the total budget, or its cost per unit
   of novelty rises for N consecutive epochs.
2. Replacement parity: the native replacement matches the transformer within tolerance
   on held-out runs. That function is retired.
3. Contribution decay: lineages with transformer provenance stop outperforming native
   lineages on novelty and activity statistics.
4. Control parity: a transformer-free branch always runs in parallel from the same
   seed. If it equals or beats the scaffolded branch, all transformer functions are
   purged.
5. Dependence: periodic test runs with transformers removed. If OEE metrics collapse,
   the system depends on them; the transformer budget is then cut further and faster,
   not raised.
6. Hard sunset: the transformer budget anneals to zero by the end of phase 2
   regardless of the other triggers.

### 4.4 Phases
- Phase 1: scaffolded bootstrap and distillation.
- Phase 2: annealing; transformer budget decreases to zero.
- Phase 3: fully native system.

## 5. Integration risks
- Imposed fitness breaks endogenous fitness. L4 and the meta-search impose outside
  objectives; Chromaria showed that violating one condition causes stagnation. Outside
  pressure must act on the environment, not on agent scores.
- Compute grows explosively. Channon's runs took years; each added layer multiplies
  cost.
- The measures disagree. Foundation-model novelty and Bedau Class 3 measure different
  things. Section 3 fixes the primary criterion.

## 6. References
- Channon, A. (2001). Passing the ALife test: activity statistics classify evolution in Geb as unbounded. ECAL 2001.
- Channon, A. (2002). Improving and still passing the ALife test: component-normalised activity statistics classify evolution in Geb as unbounded. ALife VIII.
- Channon, A. (2006). Unbounded evolutionary dynamics in a system of agents that actively process and transform their environment. GPEM. doi:10.1007/s10710-006-9009-3
- Channon, A. (2019). Maximum individual complexity is indefinitely scalable in Geb. Artificial Life 25(2):134-144. doi:10.1162/artl_a_00285
- Bedau, M., Snyder, E., Packard, N. (1998). A classification of long-term evolutionary dynamics. ALife VI.
- Soros, L., Stanley, K. (2014). Identifying necessary conditions for open-ended evolution through the artificial life world of Chromaria. ALIFE 14.
- Soros, L., Cheney, N., Stanley, K. (2016). How the strictness of the minimal criterion impacts open-ended evolution. ALIFE 2016.
- Sayama, H. (2019). Cardinality leap for open-ended evolution: theoretical consideration and demonstration by Hash Chemistry. Artificial Life. arXiv:1806.06628
- Horiguchi, Sayama (2026). Hash Chemistry: minimal models for evolutionary growth of complexity. arXiv:2607.28219
- Adams, A., Zenil, H., Davies, P., Walker, S. (2017). Formal definitions of unbounded evolution and innovation reveal universal mechanisms for open-ended evolution in dynamical systems. Sci Rep. arXiv:1607.01750
- Hernandez-Orozco, S., Hernandez-Quiroz, F., Zenil, H. The limits of decidable states on open-ended evolution and emergence. arXiv:1606.01810
- Plantec, E. et al. Flow-Lenia. arXiv:2212.07906; arXiv:2506.08569 (Artificial Life 31(2), 2025)
- Aguera y Arcas, B. et al. (2024). Computational Life. arXiv:2406.19108
- Moreno, M., Ofria, C. (2019). Toward open-ended fraternal transitions in individuality. Artificial Life 25(2). doi:10.1162/artl_a_00284; arXiv:2104.10081
- Wang, R. et al. (2020). Enhanced POET. arXiv:2003.08536
- Faldor, M. et al. OMNI-EPIC. arXiv:2405.15568
- Zhang, J. et al. (2025). Darwin Godel Machine. arXiv:2505.22954
- Kumar, A. et al. ASAL. arXiv:2412.17799; ASAL++ arXiv:2509.22447
- Akhtyrchenko, Katsnelson, Ustyuzhanin (2026). Multi-scale path divergence. arXiv:2606.17091
- Berdica, U. et al. (2026). Evolving Many Worlds (PBT-NCA). arXiv:2604.11248
- Dolson, E. et al. (2019). The MODES toolbox. Artificial Life. doi:10.1162/artl_a_00280
- Merrill, W., Sabharwal, A. The expressive power of transformers with chain of thought. arXiv:2310.07923
- Packard, N. et al. (2019). An overview of open-ended evolution. arXiv:1909.04430
- Safety must precede the deployment of open-ended AI. arXiv:2502.04512
