#!/usr/bin/env bash
# Start the run supervisors if none are running (idempotent). Used by the SessionStart hook
# so that runs resume from checkpoints after a container restart.
cd "$(dirname "$0")/.." || exit 0
if ! pgrep -f "supervise_runs.sh runs6" >/dev/null; then
  (MAX_PROCS=5 nohup tools/supervise_runs.sh runs6 runs6/bin 60 >> runs6/supervisor.out 2>&1 &)
  echo "ensure_supervisors: started runs6 supervisor"
fi
# Evolvable genetic code sweep lanes (runs7/laneN.args holds the seeds; idempotent, resume from checkpoints).
for l in runs7/lane*.args;do [ -f "$l" ]||continue;n=$(basename "$l" .args);n=${n#lane}
if ! { [ -f runs7/lane$n.pid ]&&kill -0 "$(cat runs7/lane$n.pid)" 2>/dev/null; };then
(nohup tools/bff/egc_sweep.sh $n $(cat "$l") >> runs7/lane$n.out 2>&1 &);echo "ensure_supervisors: started egc lane $n";fi;done
exit 0
