# After W4c: literature on the learner (searched 2026-11-26; every item opened at the URL given)

Question: can ES on lineage biomass find delayed, shared investments (irrigation farms)? Constraints: no
pretrained models, no demonstrations, fitness from the world only (re-attributing world payoffs is allowed).

| Source | Finding | Use here |
|---|---|---|
| Wolpert and Tumer 2001, Adv. Complex Systems 4:265, doi:10.1142/S0219525901000188 | Paying every member the world utility has poor signal-to-noise; the Wonderful Life Utility (G minus G with one's move clamped) stays aligned with G and is much less noisy | Exact WLU by same-seed replay with a lineage's writes and feeds clamped to no-op |
| Agogino and Tumer 2008, Evol. Comput. 16(2):257, doi:10.1162/evco.2008.16.2.257 | Difference evaluations beat global fitness by up to 400% in noisy multi-rover neuroevolution | Measure learnability (fitness variance from own vs others' actions) along build directions first |
| Foerster et al. 2018, arXiv:1705.08926 (COMA) | Counterfactual baseline marginalising one agent's action | The simulator gives the counterfactual exactly by replay |
| Cook and Tumer 2024, GECCO, doi:10.1145/3638529.3654157 | Learned local fitness critics aligned with global performance (GALE) | Fallback only; a learned critic needs a ruling under "only self-evolved models" |
| Li, Lee and Bareinboim 2026, arXiv:2607.16999 | Counterfactual Shapley credit with shared exogenous noise stays reliable at high noise | Build and feed events as players; one replay per sampled coalition |
| Waibel, Keller and Floreano 2009, IEEE TEC 13(3):648, doi:10.1109/TEVC.2008.2011741 | Clonal teams under team selection are not affected by credit assignment problems | ES lineages are clonal and team-selected: "shared with the lineage" is probably not the bottleneck; leakage to rivals and noise remain |
| Wang et al. 2019, arXiv:1811.05931 | In Cleanup (costly cleaning raises regrowth for all), group-evolved shared reward modules produced cooperation; individual selection did not | Same structure as irrigation |
| Taylor-Davies, Hamon, Boulet and Moulin-Frier 2024, arXiv:2411.10536 | Parental feeding evolved as Hamilton's r·b−c grew; random birth sites weakened it more than removing kin recognition | Measure r_eff, the share of farm-grown food eaten by the builder's own lineage; if r_eff·b < c, building cannot pay |
| Rosseau, Avalos and Nowé 2025, arXiv:2510.12555 | Relatedness-weighted (inclusive) rewards gave Hamilton-rule cooperation in networked PD; individual rewards gave defection | Inclusive accounting of rival biomass grown on one's own farms |
| Hamon, Sánchez-Fibla, Moulin-Frier and Solé 2026, arXiv:2605.22256 | Agriculture among PPO learners needed γ≈0.999; above ~5 agents cheaters broke it unless reward-based cloning spread farmers | Arenas with fewer competitors are a direct test |
| Perolat et al. 2017, arXiv:1707.06600 | Sustainable harvesting of a regrowing commons came mainly from excluding others | Ownership accounting does in bookkeeping what exclusion does in the world |
| Ng and Jordan 2000, arXiv:1301.3878 (PEGASUS) | Fixed random scenarios make policy evaluation deterministic | Common random numbers for antithetic partners, keyed by (entity, tick) |
| Li, Harrison, Sohl-Dickstein, Smith and Metz 2023, arXiv:2304.12180 | Noise-reuse online ES: one perturbation over a long episode, updates every truncation window; unbiased, lower variance than PES | Fixes W4c's confound: hold perturbations 2,000+ ticks, update every 600 |
| Kaya and Hashemi 2026, arXiv:2609.10980 | EGGROLL with one evaluation per direction and a leave-one-out baseline (LOO-ROLL) roughly halved estimator MSE vs antithetic pairs at equal evaluations | Compare against CRN-paired antithetic by measured gradient SNR |

Diagnostics before any mechanism (CLAUDE.md: diagnose what the world runs out of):
1. r_eff and Hamilton's rule: what share of the food grown on irrigated cells goes to the lineage whose feeds
   started the farm, against the cost of feeding.
2. Learnability: fitness variance along build/feed-heavy ES directions caused by a lineage's own actions vs by
   others' (Agogino and Tumer).
3. Single-lineage arenas against standard arenas: does building pay when no rival can eat the farm?
