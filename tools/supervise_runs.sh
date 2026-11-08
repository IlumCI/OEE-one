#!/usr/bin/env bash
# Keep a set of long Geb runs alive, and rescue frozen runs.
#
# Usage: tools/supervise_runs.sh <runs_dir> <binary_dir> [interval_s]
# Run specs are read from <runs_dir>/runs.txt, one per line:
#   <name> <variant> <seed> <stopat> [extra simulator args...]
# At most MAX_PROCS (default 4) simulations run at once, in runs.txt order.
#
# - A run whose process is not running and that has not finished is resumed from its
#   checkpoint (or started fresh).
# - Every run archives a checkpoint every 250k steps and exits with code 109 when no
#   birth occurs for 100k steps (frozen: GEB_SPEC / RESULTS "stagnation").
# - Rescue (rollback + reseed): the run is resumed from the newest archive at least
#   ROLLBACK steps before its last birth, on a new random stream. If branches from the
#   same archive refreeze MAX_TRIES times, the next older archive is used. Every rescue
#   is appended to <run>/rescues.txt. Geb's rules are unchanged; a rescued run is one
#   trajectory conditioned on not freezing (see docs/RESULTS.md).
set -uo pipefail
runs="$(realpath "$1")"; bins="$(realpath "$2")"; interval="${3:-60}"
log="$runs/supervisor.log"
ARCHIVE_EVERY="${ARCHIVE_EVERY:-250000}"
FREEZE_EXIT="${FREEZE_EXIT:-100000}"
ROLLBACK="${ROLLBACK:-250000}"
MAX_TRIES="${MAX_TRIES:-2}"
SAVE_EVERY="${SAVE_EVERY:-20000}"
MAX_PROCS="${MAX_PROCS:-4}"   # concurrent simulations; runs start in runs.txt order

rescue() {  # $1 = run dir, $2 = seed
  local d="$1" seed="$2"
  local last_birth
  last_birth=$(grep -o 'Frozen: last birth at [0-9]*' "$d/stderr.txt" | tail -1 | grep -o '[0-9]*$')
  [ -z "$last_birth" ] && return 1
  local target=$((last_birth - ROLLBACK))
  # Archives at or before target, newest first.
  local candidates
  candidates=$(ls "$d"/ck.archive.* 2>/dev/null | sed 's/.*ck\.archive\.//' | sort -n -r |
               awk -v t="$target" '$1 <= t')
  local chosen=""
  for a in $candidates; do
    local tries
    tries=0
    [ -f "$d/rescues.txt" ] && tries=$(awk -v a="$a" '$2 == a {n++} END {print n+0}' "$d/rescues.txt")
    if [ "$tries" -lt "$MAX_TRIES" ]; then chosen="$a"; break; fi
  done
  if [ -z "$chosen" ]; then
    echo "$(date -u +%FT%TZ) $(basename "$d"): no archive left to roll back to" >> "$log"
    touch "$d/unrescuable"
    return 1
  fi
  local n
  n=1
  [ -f "$d/rescues.txt" ] && n=$(( $(wc -l < "$d/rescues.txt") + 1 ))
  local reseed=$(( seed * 1000 + n ))
  cp "$d/ck.archive.$chosen" "$d/ck"
  echo "$reseed" > "$d/reseed"
  echo "$last_birth $chosen $reseed $(date -u +%FT%TZ)" >> "$d/rescues.txt"
  echo "$(date -u +%FT%TZ) $(basename "$d"): frozen after $last_birth; rollback to $chosen, reseed $reseed" >> "$log"
}

while true; do
  while read -r name variant seed stopat extra; do
    [[ -z "$name" || "$name" == \#* ]] && continue
    d="$runs/$name"
    mkdir -p "$d"
    [ -f "$d/finished" ] && continue
    [ -f "$d/unrescuable" ] && continue
    if [ -f "$d/pid" ] && grep -qa oee_geb "/proc/$(cat "$d/pid")/cmdline" 2>/dev/null; then continue; fi
    if [ -f "$d/exit_code" ]; then
      code="$(cat "$d/exit_code")"
      if [ "$code" = "100" ]; then touch "$d/finished"; continue; fi
      if [ "$code" = "109" ]; then rescue "$d" "$seed" || continue; fi
    fi
    running=0
    for pf in "$runs"/*/pid; do
      [ -f "$pf" ] && grep -qa oee_geb "/proc/$(cat "$pf")/cmdline" 2>/dev/null && running=$((running + 1))
    done
    [ "$running" -ge "$MAX_PROCS" ] && continue
    rm -f "$d/exit_code"
    resume=()
    [ -f "$d/ck" ] && resume=(-resume ck)
    if [ -f "$d/reseed" ] && [ -f "$d/ck" ]; then
      resume+=(-reseed "$(cat "$d/reseed")")
      rm -f "$d/reseed"   # applied once; a later restart resumes from newer checkpoints
    fi
    # shellcheck disable=SC2086
    (cd "$d" || exit 1; nohup bash -c "\"$bins/oee_geb_$variant\" -seed $seed -stats -rulestat -log log.txt \
        $extra -save ck -save-every $SAVE_EVERY -archive-every $ARCHIVE_EVERY -freeze-exit $FREEZE_EXIT \
        -stopat $stopat ${resume[*]} 2>>stderr.txt; echo \$? > exit_code" >/dev/null 2>&1 &
     echo $! > pid)
    sleep 1
    sim=$(pgrep -P "$(cat "$d/pid")" 2>/dev/null | head -1)
    [ -n "$sim" ] && echo "$sim" > "$d/pid"
    echo "$(date -u +%FT%TZ) started $name ${resume[*]:-fresh}" >> "$log"
  done < "$runs/runs.txt"
  [ -n "${ONCE:-}" ] && break
  sleep "$interval"
done
