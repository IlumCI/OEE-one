#!/usr/bin/env bash
# Start each run directory's cmd.txt (from inside it) when fewer than N World v1 runs are active.
# Runs with -ck resume where they stopped. Usage: setsid nohup tools/w/queue.sh N RUNDIR... &
set -u;n="$1";shift
for d in "$@";do
  while [ "$(pgrep -fc '[.][.]/oee_w_v')" -ge "$n" ];do sleep 30;done
  (cd "$d"&&setsid nohup $(sed 's/  #.*//' cmd.txt) > stderr.txt 2>&1 < /dev/null &)
  echo "$(date -u +%FT%TZ) started $d";sleep 5
done
