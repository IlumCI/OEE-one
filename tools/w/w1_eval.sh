#!/usr/bin/env bash
# W1 era progress (docs/WORLD.md §6, W1 protocol): the champion at epoch e against the one at e-D, mirrored seeds.
# Usage: tools/w/w1_eval.sh BIN RUNDIR [D=250] [seeds=16] [threads=4] [world flags, default W1 training settings]
# Output (TSV): e e_prev ishareA se t wins shareA   (ishareA: integrated share of the later champion; t against 0.5)
set -u
bin="$1";run="$2";D="${3:-250}";n="${4:-16}";T="${5:-4}";shift 5||shift $#;W="${*:--S 64 -regrow 2 -nact 7 -ticks 2000}"
printf "e\te_prev\tishareA\tse\tt\twins\tshareA\n"
for e in $(ls "$run"/archive/egg_*.bin|sed 's/.*egg_\([0-9]*\).bin/\1/'|sort -n);do
  b=$((e-D));[ $((e%D)) = 0 ]&&[ $b -ge 0 ]&&[ -f "$run/archive/egg_$b.bin" ]||continue
  "$bin" -tourney "$run/archive/egg_$e.bin" "$run/archive/egg_$b.bin" -seeds $n -threads $T $W|
  awk -v e=$e -v b=$b '/^mean_shareA/{f=$2}/^mean_ishareA/{printf "%d\t%d\t%s\t%s\t%s\t%s\t%s\n",e,b,$2,$4,$6,$8,f}'
done
