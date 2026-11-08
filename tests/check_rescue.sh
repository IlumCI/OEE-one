#!/usr/bin/env bash
# Rescue (rollback + reseed) path of tools/supervise_runs.sh, with a simulated freeze.
# Usage: tests/check_rescue.sh <bin_dir> <work_dir>
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
bins="$(realpath "$1")"; work="$2"
rm -rf "$work"; mkdir -p "$work"
echo "r1 resets 7 6000 -second-shadow -presence presence" > "$work/runs.txt"
export ARCHIVE_EVERY=1000 SAVE_EVERY=500 ROLLBACK=1500 FREEZE_EXIT=1000000 ONCE=1
# 1. Uninterrupted reference run to 6000 steps.
mkdir -p "$work/ref" && (cd "$work/ref" && "$bins/oee_geb_resets" -seed 7 -stats -rulestat -log log.txt \
  -presence presence -second-shadow -stopat 6000 2>/dev/null) || true
# 2. Supervised run to 4000 steps, then fake a freeze with last birth at 3600.
sed -i 's/ 6000 / 4000 /' "$work/runs.txt"
"$root/tools/supervise_runs.sh" "$work" "$bins" 1
while [ ! -f "$work/r1/exit_code" ]; do sleep 1; done
[ "$(cat "$work/r1/exit_code")" = "100" ] || { echo "first leg failed"; exit 1; }
ls "$work/r1"/ck.archive.* > /dev/null
rm -f "$work/r1/finished"
echo "Frozen: last birth at 3600, no birth for 400 steps" >> "$work/r1/stderr.txt"
echo 109 > "$work/r1/exit_code"
sed -i 's/ 4000 / 6000 /' "$work/runs.txt"
# 3. Supervisor must roll back to archive 2000 (newest <= 3600-1500) and reseed.
"$root/tools/supervise_runs.sh" "$work" "$bins" 1
grep -q '^3600 2000 7001 ' "$work/r1/rescues.txt" || { echo "wrong rescue record"; cat "$work/r1/rescues.txt"; exit 1; }
while [ ! -f "$work/r1/exit_code" ]; do sleep 1; done
grep -q 'Resumed from ck at time 2000' "$work/r1/stderr.txt"
grep -q 'Reseeded with 7001 at time 2000' "$work/r1/stderr.txt"
# 4. Output up to step 2000 identical to the reference; after it, a different branch.
n=$(grep -n 'time: T2000T' "$work/ref/log.txt" | cut -d: -f1)
cmp <(head -n $((n + 2)) "$work/ref/log.txt") <(head -n $((n + 2)) "$work/r1/log.txt")
if cmp -s "$work/ref/log.txt" "$work/r1/log.txt"; then echo "branch identical to reference"; exit 1; fi
grep -q 'time: T6000T' "$work/r1/log.txt"
[ "$(grep -c 'time: T2100T' "$work/r1/log.txt")" = "1" ] || { echo "output not truncated"; exit 1; }
echo "Rescue path OK"
