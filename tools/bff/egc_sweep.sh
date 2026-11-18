#!/bin/bash
# Evolvable genetic code sweep (docs/EGC.md). One lane runs its seeds in order; for each seed the fixed-code
# control (-egc 1), the evolvable code (-egc 2) and the evolvable code started from the standard code
# (-egc 2 -egc-std), paper configuration (131072 programs), up to 16384
# epochs or 2048 after a transition. Idempotent: finished runs (exit_code) are skipped, unfinished ones
# resume from their checkpoint.  Usage: tools/bff/egc_sweep.sh LANE SEED...
cd "$(dirname "$0")/../.." || exit 1
lane=$1;shift
B=runs7/bin/oee_bff_egc2
echo $$ > runs7/lane$lane.pid
for s in "$@";do for m in 1 2 3;do
# m=3: evolvable code starting from the standard code (-egc 2 -egc-std)
d=runs7/egc${m}_s$s;mkdir -p $d
x=(-egc $m);[ $m = 3 ]&&x=(-egc 2 -egc-std)
[ -f $d/exit_code ]&&continue
r=();[ -f $d/ck ]&&r=(-resume $d/ck)
$B -seed $s "${x[@]}" -epochs 16384 -interval 64 -log $d/log.csv -save $d/ck -save-every 256 -stop-entropy 1 -stop-after 2048 "${r[@]}" 2>>$d/stderr.txt
echo $? > $d/exit_code
done;done
rm -f runs7/lane$lane.pid
