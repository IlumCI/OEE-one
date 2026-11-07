#!/usr/bin/env bash
# Run a Geb binary headless and store its outputs (golden data, or a run to compare).
# Usage: tools/geb_reference/make_golden.sh <binary> <dest_dir> <seed> <stopat> [extra args...]
set -euo pipefail

bin="$(realpath "$1")"; dest="$2"; seed="$3"; stopat="$4"; shift 4
mkdir -p "$dest"
cd "$dest"
rm -f stdout.txt stderr.txt rulestat SHADOWrulestat rulestat.verifier SHADOWrulestat.verifier SHA256SUMS
rc=0
env -u DISPLAY "$bin" -seed "$seed" -stats -rulestat -stopat "$stopat" "$@" > stdout.txt 2> stderr.txt || rc=$?
# Exit code 100 is Geb's normal termination at -stopat.
if [ "$rc" -ne 100 ]; then echo "unexpected exit code $rc" >&2; exit 1; fi
echo "$*" > args.txt
sha256sum stdout.txt stderr.txt rulestat SHADOWrulestat rulestat.verifier SHADOWrulestat.verifier > SHA256SUMS
