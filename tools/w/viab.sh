#!/usr/bin/env bash
# Viability guard (docs/WORLD.md, W3.x): a run is NONVIABLE if its reseed count rose by more than 10 within the last
# 200 epochs. A run directory may hold a file guard_skip with a number of epochs whose reseeds are not counted (W4d:
# the guard excludes a startup period). Prints "name:epoch:reseeds_in_window" for each run given, with NONVIABLE
# appended when the guard trips.
for d in "$@";do [ -f "$d/log.txt" ]||continue;s=0;[ -f "$d/guard_skip" ]&&s=$(cat "$d/guard_skip")
  awk -v n="$(basename "$d")" -v s="$s" '!/^#/{e[++k]=$1;r[k]=$9}END{if(!k)exit;j=k;while(j>1&&e[k]-e[j-1]<=200)--j;while(j<k&&e[j]<s)++j;w=r[k]-r[j];printf "%s:%s:%d%s\n",n,e[k],w,(w>10?" NONVIABLE":"")}' "$d/log.txt";done
