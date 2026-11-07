#!/usr/bin/env bash
# Run every case in golden_cases.txt with binaries from <bin_dir> (geb_resets, geb_no_resets
# or a port binary invoked as "<bin_dir>/<binary_prefix>_<variant>"), writing to <out_dir>/<name>.
# Usage: tools/geb_reference/run_cases.sh <bin_prefix> <out_dir>
#   e.g. run_cases.sh build/geb_reference/geb tests/golden/geb_v08b3
set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
prefix="$1"; out="$2"
pids=()
while read -r name variant seed stopat extra; do
  [[ -z "$name" || "$name" == \#* ]] && continue
  # shellcheck disable=SC2086
  "$here/make_golden.sh" "${prefix}_${variant}" "$out/$name" "$seed" "$stopat" $extra &
  pids+=($!)
done < "$here/golden_cases.txt"
rc=0
for p in "${pids[@]}"; do wait "$p" || rc=1; done
exit $rc
