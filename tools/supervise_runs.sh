#!/usr/bin/env bash
# Keep a set of long Geb runs alive: any run whose process is not running and that
# has not reached its stop step is resumed from its checkpoint (or started fresh).
# Usage: tools/supervise_runs.sh <runs_dir> <binary_dir> <stopat> [interval_s]
# Run specs are read from <runs_dir>/runs.txt: "<name> <variant> <seed> <extra args...>"
set -uo pipefail
runs="$(realpath "$1")"; bins="$(realpath "$2")"; stopat="$3"; interval="${4:-60}"
log="$runs/supervisor.log"
while true; do
  while read -r name variant seed extra; do
    [[ -z "$name" || "$name" == \#* ]] && continue
    d="$runs/$name"
    mkdir -p "$d"
    [ -f "$d/finished" ] && continue
    if [ -f "$d/pid" ] && grep -qa oee_geb "/proc/$(cat "$d/pid")/cmdline" 2>/dev/null; then continue; fi
    # Not running: was it finished?
    if [ -f "$d/exit_code" ] && [ "$(cat "$d/exit_code")" = "100" ]; then touch "$d/finished"; continue; fi
    rm -f "$d/exit_code"
    resume=()
    [ -f "$d/ck" ] && resume=(-resume ck)
    # shellcheck disable=SC2086
    (cd "$d" || exit 1; nohup bash -c "\"$bins/oee_geb_$variant\" -seed $seed -stats -rulestat -log log.txt \
        -components components -presence presence $extra -save ck -save-every 20000 \
        -stopat $stopat ${resume[*]} 2>>stderr.txt; echo \$? > exit_code" >/dev/null 2>&1 &
     echo $! > pid)
    sleep 1
    # Record the actual simulator pid (child of the bash wrapper).
    sim=$(pgrep -P "$(cat "$d/pid")" 2>/dev/null | head -1)
    [ -n "$sim" ] && echo "$sim" > "$d/pid"
    echo "$(date -u +%FT%TZ) started $name ${resume[*]:-fresh}" >> "$log"
  done < "$runs/runs.txt"
  sleep "$interval"
done
