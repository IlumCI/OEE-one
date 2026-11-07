#!/usr/bin/env bash
# Run the port on every golden case and require byte-identical outputs.
# Usage: tests/check_golden.sh <bin_prefix> <golden_dir> <work_dir>
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
prefix="$1"; golden="$2"; work="$3"
rm -rf "$work"; mkdir -p "$work"
"$root/tools/geb_reference/run_cases.sh" "$prefix" "$work"
fail=0
for d in "$golden"/*/; do
  name="$(basename "$d")"
  if (cd "$d" && sha256sum --quiet -c SHA256SUMS --status 2>/dev/null); then :; fi
  for f in stdout.txt stderr.txt rulestat SHADOWrulestat rulestat.verifier SHADOWrulestat.verifier; do
    if ! cmp -s "$d/$f" "$work/$name/$f"; then
      echo "MISMATCH $name/$f"
      cmp "$d/$f" "$work/$name/$f" | head -1 || true
      fail=1
    fi
  done
done
if [ "$fail" -eq 0 ]; then echo "All golden cases identical"; fi
exit $fail
