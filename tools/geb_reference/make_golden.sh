#!/usr/bin/env bash
# Run the reference binary headless and store outputs as golden data for the port.
# Usage: tools/geb_reference/make_golden.sh <binary> <seed> <stopat> <dest_dir>
set -euo pipefail

bin="$(realpath "$1")"; seed="$2"; stopat="$3"; dest="$4"
mkdir -p "$dest"
cd "$dest"
rc=0
env -u DISPLAY "$bin" -seed "$seed" -stats -rulestat -stopat "$stopat" > stdout.txt 2>&1 || rc=$?
# Exit code 100 is Geb's normal termination at -stopat.
if [ "$rc" -ne 100 ]; then echo "unexpected exit code $rc" >&2; exit 1; fi
sha256sum stdout.txt rulestat SHADOWrulestat > SHA256SUMS
