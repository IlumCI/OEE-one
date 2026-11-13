#!/usr/bin/env bash
# Start the run supervisors if none are running (idempotent). Used by the SessionStart hook
# so that runs resume from checkpoints after a container restart.
cd "$(dirname "$0")/.." || exit 0
if ! pgrep -f "supervise_runs.sh runs6" >/dev/null; then
  (MAX_PROCS=4 nohup tools/supervise_runs.sh runs6 runs6/bin 60 >> runs6/supervisor.out 2>&1 &)
  echo "ensure_supervisors: started runs6 supervisor"
fi
exit 0
