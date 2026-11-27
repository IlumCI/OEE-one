#!/usr/bin/env bash
# Agent study, part 2 (docs/WORLD.md, agent study protocol): shuffle lesions (each agent of A sees another agent's
# values in the group: same distribution, own information lost) and the gates (futile draws drawn again: the
# headroom of a flaw), the champion at epoch e against an intact copy, in the run's training world.
# Usage: tools/w/agents2.sh BIN RUNDIR EPOCH OUTFILE [seeds=16] [threads=1]
set -u;bin="$1";r="$2";e="$3";o="$4";n="${5:-16}";T="${6:-1}";d=$(dirname "$0");W=$("$d"/wflags.sh "$r");g="$r/archive/egg_$e.bin"
{ echo "# epoch $e ($n seeds x 2): test entries mean_lshareA se t wins"
for m in food:0-24 agents:25-49 energy:50 age:51 symbols:53-78 material:79-103 tape:104-120 wpos:121 land:122 cue:123-126 blind:0-51,53-127;do
  nm=${m%%:*};rg=${m#*:};echo "$W"|grep -q -- '-home_cue 1'||[ $nm != cue ]||continue
  "$bin" -tourney "$g" "$g" -seeds $n -threads $T -mask $rg -mask_mode shuffle $W|awk -v nm=shuffle_$nm -v rg=$rg '/^mean_lshareA/{printf "%s\t%s\t%s\t%s\t%s\t%s\n",nm,rg,$2,$4,$6,$8}';done
for ab in gate_eat gate_all;do "$bin" -tourney "$g" "$g" -seeds $n -threads $T -ablate $ab $W|awk -v nm=$ab '/^mean_lshareA/{printf "%s\t-\t%s\t%s\t%s\t%s\n",nm,$2,$4,$6,$8}';done; } > "$o"
