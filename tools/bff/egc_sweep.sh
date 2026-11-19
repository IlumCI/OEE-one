#!/bin/bash
# Evolvable genetic code sweep (docs/EGC.md). One lane runs its seeds in order; for each seed the fixed-code
# control (-egc 1), the evolvable code (-egc 2) and the evolvable code started from the standard code
# (-egc 2 -egc-std), paper configuration (131072 programs), up to 16384
# epochs or 2048 after a transition. Idempotent: finished runs (exit_code) are skipped, unfinished ones
# resume from their checkpoint.  Usage: tools/bff/egc_sweep.sh LANE SEED...
cd "$(dirname "$0")/../.." || exit 1
lane=$1;shift
B=runs7/bin/oee_bff_egc5
echo $$ > runs7/lane$lane.pid
# Arms: 1 fixed code; 2 evolvable, random tables, no guard; 8 evolvable, random tables, guard 15; 9 evolvable,
# standard start, guard 15 (guard 15: heads after table, no +/- on tables, aligned copies only, tables
# mutate 256x slower). Arms 3-7 (weaker guards) only where already started.
# Option 2 (external tables inherited by authorship; its control is plain BFF, docs/M3.md): 10 standard start,
# 11 random start. Option 3 (256-entry maps, synonymous codons): 12 standard start, 13 random. ARMS (env or runs7/laneN.arms) picks the arms; default 1-9.
[ -z "$ARMS" ]&&[ -f runs7/lane$lane.arms ]&&ARMS=$(cat runs7/lane$lane.arms)
ARMS=${ARMS:-1 2 3 4 5 6 7 8 9}
for s in "$@";do for m in $ARMS;do
[ -f runs7/lane$lane.stop ]&&break 2
case $m in 3|4|5|6|7)[ ! -d runs7/egc${m}_s$s ]&&continue;;esac
d=runs7/egc${m}_s$s;mkdir -p $d
case $m in 1)x=(-egc 1);;2)x=(-egc 2);;3)x=(-egc 2 -egc-std);;4)x=(-egc 2 -egc-guard 3);;5)x=(-egc 2 -egc-std -egc-guard 3);;6)x=(-egc 2 -egc-guard 7);;7)x=(-egc 2 -egc-std -egc-guard 7);;8)x=(-egc 2 -egc-guard 15);;9)x=(-egc 2 -egc-std -egc-guard 15);;10)x=(-egc 4 -egc-std);;11)x=(-egc 4);;12)x=(-egc 5 -egc-std);;13)x=(-egc 5);;esac
b=$B;[ $m -ge 10 ]&&b=runs7/bin/oee_bff_egc7;[ $m -ge 12 ]&&b=runs7/bin/oee_bff_egc8
[ -f $d/exit_code ]&&continue
r=();[ -f $d/ck ]&&r=(-resume $d/ck)
# Standard-start arms (3, 5, 7, 9): identical tables alone add ~0.9 to high-order entropy, so the H >= 1
# stop would fire without replicators; they run the full 16384 epochs (judge replication by selfrep share).
st=(-stop-entropy 1 -stop-after 2048);case $m in 3|5|7|9)st=();;esac
$b -seed $s "${x[@]}" -epochs 16384 -interval 64 -log $d/log.csv -save $d/ck -save-every 256 "${st[@]}" "${r[@]}" 2>>$d/stderr.txt
echo $? > $d/exit_code
done;done
rm -f runs7/lane$lane.pid
