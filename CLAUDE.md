# Operating rules for this repository

## Long runs (mandatory)
- While any simulation is running, a harness-tracked Monitor must ALWAYS be armed
  (20-minute status loop, timeout 30 min). Re-arm it immediately on every expiry notice.
  Never replace it with a one-off background job; a one-off job may run in addition.
  If nothing is tracked, the container is reclaimed and all runs stop.
- After any container restart (uptime near 0, supervisor: 0), restart supervisors:
  (MAX_PROCS=1 nohup tools/supervise_runs.sh runs3 runs3/bin 60 >> runs3/supervisor.out 2>&1 &)
  (MAX_PROCS=3 nohup tools/supervise_runs.sh runs6 runs6/bin 60 >> runs6/supervisor.out 2>&1 &)
- Keep working between events; do not idle.

## Commits
- Author: ilumCI <ilumbackup@gmail.com>. No trailers, no co-author lines, no tool mentions.
- Branch: ccr-a2d38bd7-kjgb66.
