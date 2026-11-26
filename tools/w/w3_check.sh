#!/usr/bin/env bash
# W3.x pilot and wave-2 checks (docs/WORLD.md §6), at epoch e in the run's training world (tools/w/wflags.sh):
#  1. the champion against random-weight agents (archive/egg_0.bin) with -stats: action fractions per agent-tick
#     (make counts make and unmake), joint harvests per 1000 agent-ticks;
#  2. ablations, the champion against its intact copy (mirrored seeds): wipe (A's machines removed every 100 ticks)
#     and deaf (with signals). Shares of the ablated copy: ishareA se t lshareA lse lt; < 0.5 with t < -3 means the
#     mechanism pays.
# Usage: tools/w/w3_check.sh BIN RUNDIR EPOCH [seeds=16] [threads=2]
set -u
bin="$1";run="$2";e="$3";n="${4:-16}";T="${5:-2}";W=$("$(dirname "$0")"/wflags.sh "$run")
g="$run/archive/egg_$e.bin";r="$run/archive/egg_0.bin";echo "# $run e $e world: $W"
"$bin" -tourney "$g" "$r" -seeds $n -threads $T -stats 1 $W|grep '^stats'|sed 's/^stats A/champion/;s/^stats B/random/'
for ab in wipe deaf;do [ $ab = deaf ]&&[[ " $W " != *" -signals 1 "* ]]&&continue
  "$bin" -tourney "$g" "$g" -seeds $n -threads $T -ablate $ab $W|
  awk -v a=$ab '/^mean_ishareA/{i=$2"\t"$4"\t"$6}/^mean_lshareA/{printf "%s\t%s\t%s\t%s\t%s\n",a,i,$2,$4,$6}'
done
