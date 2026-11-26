#!/usr/bin/env bash
# Viability guard (docs/WORLD.md, W3.x): a run is NONVIABLE if its reseed count rose by more than 10 within the last
# 200 epochs. Prints "name:epoch:reseeds_in_window" for each run given, with NONVIABLE appended when the guard trips.
for d in "$@";do [ -f "$d/log.txt" ]||continue
  awk -v n="$(basename "$d")" '!/^#/{e[++k]=$1;r[k]=$9}END{if(!k)exit;j=k;while(j>1&&e[k]-e[j-1]<=200)--j;w=r[k]-r[j];printf "%s:%s:%d%s\n",n,e[k],w,(w>10?" NONVIABLE":"")}' "$d/log.txt";done
