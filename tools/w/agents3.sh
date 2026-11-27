#!/usr/bin/env bash
# Agent study across runs (docs/WORLD.md, agent study): for RUNDIR:EPOCH pairs, the champion and egg_0 in self-play
# (probe records, tools/w/probe.py), the champion's input sensitivity on its own play (sens), the gates (futile eats /
# all futile actions drawn again, against an intact copy) and the shuffle lesion of food, in each run's training world.
# Usage: tools/w/agents3.sh BIN OUTDIR [seeds=16] RUNDIR:EPOCH...
set -u;bin="$1";o="$2";n="${3:-16}";shift 3;d=$(dirname "$0");S="${SENS:-build/rel/sens}";mkdir -p "$o"
for x in "$@";do r=${x%%:*};e=${x#*:};b=$(basename $r);W=$("$d"/wflags.sh "$r");a="$r/archive"
  for g in 0 $e;do "$bin" -tourney "$a/egg_$g.bin" "$a/egg_$g.bin" -seeds 2 -threads 1 -probe "$o/${b}_e$g.prb" -probe_games 4 $W >/dev/null;done
  python3 "$d"/probe.py -names ${b}_0,${b}_$e "$o/${b}_e0.prb" "$o/${b}_e$e.prb" > "$o/${b}_probe.txt"
  "$S" "$a/egg_$e.bin" "$o/${b}_e$e.prb" -n 20000 > "$o/${b}_sens.txt"
  { for ab in gate_eat gate_all;do "$bin" -tourney "$a/egg_$e.bin" "$a/egg_$e.bin" -seeds $n -threads 1 -ablate $ab $W|awk -v nm=$ab '/^mean_lshareA/{printf "%s\t%s\t%s\t%s\t%s\n",nm,$2,$4,$6,$8}';done
    "$bin" -tourney "$a/egg_$e.bin" "$a/egg_$e.bin" -seeds $n -threads 1 -mask 0-24 -mask_mode shuffle $W|awk '/^mean_lshareA/{printf "shuffle_food\t%s\t%s\t%s\t%s\n",$2,$4,$6,$8}'; } > "$o/${b}_gates.txt"
  rm -f "$o/${b}_e0.prb";done
