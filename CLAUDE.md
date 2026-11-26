# Operating rules for this repository

## Direction (read first; binding)
- Goal: open-ended evolution toward ASI. Plan and rules: docs/PLAN.md §0–4. Current build: docs/WORLD.md
  (World v1, phases W0–W4; main line now W3 programmable matter, re-plan 2026-11-25). Engine: src/evo (integer
  EGGROLL on AMX int8), docs/EVO.md.
- Containment (binding): evolved agents never get real network, device or internet access, directly or through
  anything they build. Digital richness comes from substrates emulated inside the sandbox.
- No human knowledge as a training signal:
  - no pretrained models in the system or its loop (open or proprietary, local or remote);
  - only self-evolved models;
  - local compute only.
- Fitness comes from the world only.
- Measure progress without humans, always against a control:
  - ancestor tournaments (docs/TOURNEY.md);
  - held-out generated arenas;
  - ablations.
- Before each experiment, write down its kill criterion. When progress flattens, diagnose what the world runs out
  of before adding mechanisms. Do not keep adding mechanisms to the same substrate.
- The Geb ladder (L2–L4) is frozen: do not start new Geb variants.

## Long runs (mandatory)
- While any simulation is running, a harness-tracked Monitor must ALWAYS be armed
  (20-minute status loop, timeout 30 min). Re-arm it immediately on every expiry notice.
  Never replace it with a one-off background job; a one-off job may run in addition.
  If nothing is tracked, the container is reclaimed and all runs stop.
- After any container restart (uptime near 0, supervisor: 0), run tools/ensure_supervisors.sh
  (idempotent), i.e.:
  (runs3: all L80 runs paused at checkpoints; uncomment in runs3/runs.txt and start
   MAX_PROCS=1 nohup tools/supervise_runs.sh runs3 runs3/bin 60 to resume)
  (MAX_PROCS=11 nohup tools/supervise_runs.sh runs6 runs6/bin 60 >> runs6/supervisor.out 2>&1 &)
  (runs6: L4 scale runs l4L* paused 2026-11-25 at 1.60-1.96M steps; to resume, uncomment them in runs6/runs.txt)
  (runs8: World v1 runs are not supervised. After a restart resume them all, 4 at a time (bit-exact from ck.bin;
   finished runs exit at once): setsid nohup tools/w/queue.sh 4 runs8/w1_* runs8/w1b_* runs8/w1c_* runs8/w1d_* runs8/w1e_* runs8/w2_* runs8/w3* >> runs8/queue.log 2>&1 &)
- Keep working between events; do not idle.

## Commits
- Author: ilumCI <ilumbackup@gmail.com>. No trailers, no co-author lines, no tool mentions.
- Branch: main (renamed from ccr-a2d38bd7-kjgb66 by the owner). Raw run data: branch run-data.
- Commit messages: short and casual, the way a developer writes them in a hurry
  (e.g. "fix", "wip", "more tests", "update docs", "speed up sensing", "tweak runs").
  Lower case, no detailed summaries, no result numbers in the subject.

## Research method
- Consult arXiv for current work before design decisions.
- If a specific search returns little, generalise the problem (strip the domain specifics, state the
  abstract problem) and search the fields that have solved the general version; then map the
  solution back.

## Code style (owner's)
- No spaces around operators: `a=b+c*d`, `if(x<y&&z!=0)`, `for(int i=0;i<n;++i)`.
- Skip indentation where the syntax allows it (C++: put short bodies on one line, e.g.
  `if(!o)continue;`, `for(auto&x:v)s+=x;`). Python keeps the indentation it requires.
- Use compact or abbreviated names when a function or variable name gets long
  (e.g. `copy_net_inputs_from_neighbourhood` -> `sense`, `num_individuals` -> `nind`).
- Applies to new and edited code; existing code is not mass-reformatted.

## Storage
- Finished runs are archived to branch `run-data` automatically by the supervisor (AUTO_ARCHIVE=1;
  tools/storage/archive_runs.sh, log in <runs>/archive.log; a run followed by an unfinished yoked run waits)
  and replaced by a stub (finished, exit_code, ARCHIVED). KEEP=1 uploads a copy without removing. Restore before analysis or resume: tools/storage/restore_run.sh RUNDIR.
- presence of finished runs may be packed (presence.xz, tools/storage/pack_runs.sh); analysis reads both,
  unpack before resuming.
