#!/usr/bin/env bash
# Keep copies of a run's checkpoint (RUNDIR/cks/ck_<epoch>.bin) every 100 epochs, and at every checkpoint (10 epochs)
# while the run is booming (last log line: more than 150 agents or more than 200 machines). For -arena_ck analyses of
# constructed niches (docs/WORLD.md, W4a). Polls every 60 s until all runs reach their -epochs.
# Usage: tools/w/boomsnap.sh RUNDIR...
set -u
ckep(){ od -An -t d8 -j 8 -N 8 "$1"|tr -d ' '|{ read n;od -An -t d8 -j $((16+8*n)) -N 8 "$1"|tr -d ' '; }; }
while :;do left=0
  for d in "$@";do [ -f "$d/log.txt" ]&&[ -f "$d/ck.bin" ]||{ left=1;continue; }
    tot=$(grep -o -- '-epochs [0-9]*' "$d/cmd.txt"|head -1|awk '{print $2}')
    read e p m < <(awk '!/^#/{e=$1;p=$2;m=$(NF-3)}END{print e+0,p+0,m+0}' "$d/log.txt")
    [ "$e" -lt "$tot" ]&&left=1
    c=$(ckep "$d/ck.bin" 2>/dev/null)||continue;[ -n "$c" ]||continue;mkdir -p "$d/cks"
    if [ $((c%100)) = 0 ]||[ "$p" -gt 150 ]||[ "$m" -gt 200 ];then [ -f "$d/cks/ck_$c.bin" ]||cp "$d/ck.bin" "$d/cks/ck_$c.bin";fi
  done;[ $left = 0 ]&&break;sleep 60;done
