#!/usr/bin/env bash
# W0 pass test (docs/WORLD.md §6): archived means vs the initial mean in shared arenas, 5 seeds each, plus the
# same-genome drift control, and progress between eras (epoch e vs e-100; the informative measure, since the
# initial mean cannot survive on its own). Usage: tools/w/w0_eval.sh BIN RUNDIR [ticks=2000] [extra world args, e.g. -S 128 -regrow 2]
set -u
bin="$1";run="$2";ticks="${3:-2000}";shift 3||shift $#;extra="$*"
last=$(ls "$run"/archive/egg_*.bin|sed 's/.*egg_\([0-9]*\).bin/\1/'|sort -n|tail -1)
echo "# W0 eval: run $run, last epoch $last, ticks $ticks, world args: $extra"
for e in $(ls "$run"/archive/egg_*.bin|sed 's/.*egg_\([0-9]*\).bin/\1/'|sort -n);do
  [ "$e" = 0 ]&&continue
  m=$("$bin" -tourney "$run/archive/egg_$e.bin" "$run/archive/egg_0.bin" -seeds 5 -ticks $ticks $extra|awk '/mean_shareA/{print $2}')
  echo "epoch $e vs 0: shareA $m"
done
echo "# progress between eras (epoch e vs e-100):"
for e in $(ls "$run"/archive/egg_*.bin|sed 's/.*egg_\([0-9]*\).bin/\1/'|sort -n);do
  b=$((e-100));[ $b -lt 25 ]&&continue;[ -f "$run/archive/egg_$b.bin" ]||continue
  m=$("$bin" -tourney "$run/archive/egg_$e.bin" "$run/archive/egg_$b.bin" -seeds 5 -ticks $ticks $extra|awk '/mean_shareA/{print $2}')
  echo "epoch $e vs $b: shareA $m"
done
echo "control (epoch $last vs itself):"
"$bin" -tourney "$run/archive/egg_$last.bin" "$run/archive/egg_$last.bin" -seeds 5 -ticks $ticks $extra
echo "final (epoch $last vs 0), per seed:"
"$bin" -tourney "$run/archive/egg_$last.bin" "$run/archive/egg_0.bin" -seeds 5 -ticks $ticks $extra
