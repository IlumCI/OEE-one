#!/usr/bin/env bash
# Start each run directory's cmd.txt (from inside it) when fewer than N World v1 runs are active.
# Runs with -ck resume where they stopped. A directory whose run is already active, or whose log already reached the
# epochs in cmd.txt, is skipped. Usage: setsid nohup tools/w/queue.sh N RUNDIR... &
set -u;n="$1";shift
active(){ for p in $(pgrep -f '[.][.]/oee_w_v');do [ "$(readlink /proc/$p/cwd)" = "$(realpath "$1")" ]&&return 0;done;return 1; }
for d in "$@";do
  tot=$(grep -o -- '-epochs [0-9]*' "$d/cmd.txt"|head -1|awk '{print $2}');last=$( [ -f "$d/log.txt" ]&&awk '!/^#/{e=$1}END{print e+0}' "$d/log.txt"||echo 0)
  if active "$d";then echo "$(date -u +%FT%TZ) skip $d (active)";continue;fi
  if [ "$last" -ge "$tot" ];then echo "$(date -u +%FT%TZ) skip $d (done)";continue;fi
  while [ "$(pgrep -fc '[.][.]/oee_w_v')" -ge "$n" ];do sleep 30;done
  (cd "$d"&&setsid nohup $(sed 's/  #.*//' cmd.txt) > stderr.txt 2>&1 < /dev/null &)
  echo "$(date -u +%FT%TZ) started $d";sleep 5
done
