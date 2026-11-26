#!/usr/bin/env bash
# Run tools/w/w3_check.sh on each run as soon as its champion of epoch E is archived; append to OUT.
# Usage: tools/w/w3_when.sh BIN E OUT RUNDIR...
set -u;bin="$1";e="$2";out="$3";shift 3;left=("$@")
while [ ${#left[@]} -gt 0 ];do nl=()
  for r in "${left[@]}";do if [ -f "$r/archive/egg_$e.bin" ];then "$(dirname "$0")"/w3_check.sh "$bin" "$r" "$e" 16 1 >> "$out";echo "done $r";else nl+=("$r");fi;done
  left=("${nl[@]+"${nl[@]}"}");[ ${#left[@]} -gt 0 ]&&sleep 60;done
