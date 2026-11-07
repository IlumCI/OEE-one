#!/usr/bin/env bash
# One-line-per-run status of supervised Geb runs.
# Usage: tools/run_status.sh <runs_dir>
runs="${1:-runs}"
echo "uptime: $(uptime -p)  supervisor: $(pgrep -fc supervise_runs.sh)  sims: $(pgrep -fc 'bin_ckpt/oee_geb')  disk: $(df -h --output=avail "$runs" | tail -1 | tr -d ' ')"
while read -r name _; do
  [[ -z "$name" || "$name" == \#* ]] && continue
  d="$runs/$name"
  t=$(tail -1 "$d/log.txt" 2>/dev/null | grep -o 'T[0-9]*T\|L[0-9]*L\|S[0-9]*S' | head -1 | tr -d 'TLS')
  ck=$( [ -f "$d/ck" ] && date -u -r "$d/ck" +%H:%M:%S || echo none)
  alive=$( [ -f "$d/pid" ] && grep -qa oee_geb "/proc/$(cat "$d/pid")/cmdline" 2>/dev/null && echo up || echo DOWN)
  echo "$name step=${t:-?} checkpoint=$ck $alive$( [ -f "$d/finished" ] && echo ' finished')"
done < "$runs/runs.txt"
tail -3 "$runs/supervisor.log" 2>/dev/null
