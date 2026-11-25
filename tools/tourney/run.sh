#!/usr/bin/env bash
# Tournament batch (docs/TOURNEY.md): later vs earlier populations of the same run, plus same-era drift controls.
# Usage: tools/tourney/run.sh BIN OUTDIR RUN... (each RUN a run directory with ck.archive.<step> files)
set -u
bin="$(realpath "$1")";out="$2";shift 2;mkdir -p "$out"
T=3000;L=32;N=128;E=500
for R in "$@";do r=$(basename "$R")
 pairs="500000:250000 750000:500000 1000000:750000 1250000:1000000 1500000:1250000 1500000:250000 1500000:1500000 250000:250000"
 for s in 1 2 3;do
  for pr in $pairs;do a=${pr%:*};b=${pr#*:};f="$out/${r}_${a}_${b}_s$s.tsv";[ -s "$f" ]&&continue
   nice -n 10 "$bin" "$R/ck.archive.$a" "$R/ck.archive.$b" $T $s $L $N $E > "$f.tmp" 2>/dev/null&&mv "$f.tmp" "$f";done
  f="$out/${r}_1500000_250000_n84_s$s.tsv";[ -s "$f" ]||{ nice -n 10 "$bin" "$R/ck.archive.1500000" "$R/ck.archive.250000" $T $s $L $N $E 84 > "$f.tmp" 2>/dev/null&&mv "$f.tmp" "$f";}
 done;done
echo done
