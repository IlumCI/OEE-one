#!/usr/bin/env bash
# Compact status of supervised Geb runs.
# Usage: tools/run_status.sh <runs_dir>
runs="${1:-runs}"
sims=$(pgrep -fc "^/[^ ]*/bin[^/ ]*/oee_geb")
sup=$(pgrep -fc '^bash .*tools/supervise_runs.sh')
total=0; fin=0; up=0; unres=0; rescues=0
lines=""
while read -r name _ _ stopat _; do
  [[ -z "$name" || "$name" == \#* ]] && continue
  total=$((total + 1))
  d="$runs/$name"
  [ -f "$d/finished" ] && { fin=$((fin + 1)); continue; }
  [ -f "$d/unrescuable" ] && unres=$((unres + 1))
  [ -f "$d/rescues.txt" ] && rescues=$((rescues + $(wc -l < "$d/rescues.txt")))
  if [ -f "$d/pid" ] && grep -qa oee_geb "/proc/$(cat "$d/pid")/cmdline" 2>/dev/null; then
    up=$((up + 1))
    t=$(grep 'time: T' "$d/log.txt" 2>/dev/null | tail -1 | grep -o 'T[0-9]*T' | tr -d T)
    n=$(grep 'time: T' "$d/log.txt" 2>/dev/null | tail -1 | grep -o 'N[0-9]*N' | tr -d N)
    lines="$lines $name=${t:-0}/${stopat}(N${n:-?})"
  fi
done < "$runs/runs.txt"
for d in "$runs"/*/; do [ -f "$d/rescues.txt" ] && [ -f "$d/finished" ] && rescues=$((rescues + $(wc -l < "$d/rescues.txt"))); done
echo "uptime: $(uptime -p)  supervisor: $sup  sims: $sims  disk: $(df -h --output=avail "$runs" | tail -1 | tr -d ' ')"
echo "runs: $total total, $fin finished, $up running, $unres unrescuable, $rescues rescues so far"
echo "running:$lines"
tail -2 "$runs/supervisor.log" 2>/dev/null
