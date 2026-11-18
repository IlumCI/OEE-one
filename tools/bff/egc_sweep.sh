#!/bin/bash
# Evolvable genetic code sweep (docs/EGC.md). One lane runs its seeds in order; for each seed the fixed-code
# control (-egc 1), the evolvable code (-egc 2) and the evolvable code started from the standard code
# (-egc 2 -egc-std), paper configuration (131072 programs), up to 16384
# epochs or 2048 after a transition. Idempotent: finished runs (exit_code) are skipped, unfinished ones
# resume from their checkpoint.  Usage: tools/bff/egc_sweep.sh LANE SEED...
cd "$(dirname "$0")/../.." || exit 1
lane=$1;shift
B=runs7/bin/oee_bff_egc3
echo $$ > runs7/lane$lane.pid
# Arms: 1 fixed code; 2 evolvable, random tables, no guard; 4 evolvable, random tables, guard 3; 5 evolvable,
# standard start, guard 3. Arm 3 (standard start, no guard) only where it was already started (seed 0).
for s in "$@";do for m in 1 2 3 4 5;do
[ $m = 3 ]&&[ ! -d runs7/egc3_s$s ]&&continue
d=runs7/egc${m}_s$s;mkdir -p $d
case $m in 1)x=(-egc 1);;2)x=(-egc 2);;3)x=(-egc 2 -egc-std);;4)x=(-egc 2 -egc-guard 3);;5)x=(-egc 2 -egc-std -egc-guard 3);;esac
[ -f $d/exit_code ]&&continue
r=();[ -f $d/ck ]&&r=(-resume $d/ck)
$B -seed $s "${x[@]}" -epochs 16384 -interval 64 -log $d/log.csv -save $d/ck -save-every 256 -stop-entropy 1 -stop-after 2048 "${r[@]}" 2>>$d/stderr.txt
echo $? > $d/exit_code
done;done
rm -f runs7/lane$lane.pid
