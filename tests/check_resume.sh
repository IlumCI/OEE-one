#!/usr/bin/env bash
# Checkpoint/resume equivalence: an interrupted run (stopped after the last checkpoint,
# leaving extra output) resumed from its checkpoint must reproduce an uninterrupted
# run byte-for-byte, for both shadow variants.
# Usage: tests/check_resume.sh <bin_prefix> <work_dir>
set -euo pipefail
prefix="$1"; work="$2"
rm -rf "$work"; mkdir -p "$work"
fail=0
for v in resets no_resets; do
  bin="$(realpath "${prefix}_${v}")"
  opts=(-seed 9 -stats -rulestat -log log.txt -components components -second-shadow -presence presence)
  mkdir -p "$work/$v/A" "$work/$v/B"
  (cd "$work/$v/A" && "$bin" "${opts[@]}" -stopat 4000 2>/dev/null) || [ $? -eq 100 ]
  (cd "$work/$v/B" && "$bin" "${opts[@]}" -stopat 2500 -save ck -save-every 1000 2>/dev/null) || [ $? -eq 100 ]
  (cd "$work/$v/B" && "$bin" "${opts[@]}" -stopat 4000 -save ck -save-every 1000 -resume ck 2>/dev/null) || [ $? -eq 100 ]
  for f in log.txt rulestat SHADOWrulestat rulestat.verifier SHADOWrulestat.verifier components presence; do
    if ! cmp -s "$work/$v/A/$f" "$work/$v/B/$f"; then echo "MISMATCH $v/$f"; fail=1; fi
  done
done
[ "$fail" -eq 0 ] && echo "Resume equivalent"
exit $fail
