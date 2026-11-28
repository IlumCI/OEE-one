#!/usr/bin/env bash
# Viability guard over every window (docs/WORLD.md: more than 10 reseeds within ANY 200 epochs is non-viable): prints
# run:last_epoch:max_reseeds_in_any_200_epochs, with NONVIABLE when above 10. viab.sh checks only the latest window.
for d in "$@";do [ -f "$d/log.txt" ]||continue
  awk -v n="$(basename "$d")" '!/^#/{e[++k]=$1;r[k]=$9}END{m=0;j=1;for(i=1;i<=k;++i){while(e[i]-e[j]>200)++j;w=r[i]-(j>1?r[j-1]:0);if(w>m)m=w}printf "%s:%s:%d%s\n",n,e[k],m,(m>10?" NONVIABLE":"")}' "$d/log.txt";done
