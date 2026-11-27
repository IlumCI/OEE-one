#!/usr/bin/env bash
# Agent study of one run (docs/WORLD.md, agent study) at epoch e, in the run's training world (wflags.sh), each genome
# in self-play (A = B, so each plays in a world of its own kind):
#  1. probe records of the initial genome (egg_0), the champion at e/2 and the champion at e (tools/w/probe.py);
#  2. weight health against egg_0, and the consistency of successive changes (tools/w/eggstat.py);
#  3. input lesions: the champion with one observation group zeroed against an intact copy (mean_lshareA < 0.5 and
#     t < -3: the group is used), plus a blind copy (every entry but the bias zeroed).
# Usage: tools/w/agents.sh BIN RUNDIR EPOCH OUTDIR [seeds=16] [threads=1] [probe seeds=2]
set -u;bin="$1";r="$2";e="$3";o="$4";n="${5:-16}";T="${6:-1}";ps="${7:-2}";d=$(dirname "$0")
W=$("$d"/wflags.sh "$r");a="$r/archive";h=$((e/2/25*25));mkdir -p "$o"
for x in 0 $h $e;do g="$a/egg_$x.bin";"$bin" -tourney "$g" "$g" -seeds $ps -threads $T -probe "$o/e$x.prb" -probe_games $((2*ps)) $W >/dev/null;done
python3 "$d"/probe.py -names egg_0,egg_$h,egg_$e "$o/e0.prb" "$o/e$h.prb" "$o/e$e.prb" > "$o/probe.txt"
python3 "$d"/eggstat.py -groups "$a/egg_0.bin" "$a/egg_$e.bin" > "$o/weights.txt"
python3 "$d"/eggstat.py -drift "$a/egg_0.bin" "$a/egg_$h.bin" "$a/egg_$e.bin" >> "$o/weights.txt"
g="$a/egg_$e.bin";{ echo "# input lesions at epoch $e ($n seeds x 2): group entries mean_lshareA se t wins"
for m in food:0-24 agents:25-49 energy:50 age:51 symbols:53-78 material:79-103 tape:104-120 wpos:121 land:122 cue:123-126 blind:0-51,53-127;do
  nm=${m%%:*};rg=${m#*:};echo "$W"|grep -q -- '-home_cue 1'||[ $nm != cue ]||continue
  "$bin" -tourney "$g" "$g" -seeds $n -threads $T -mask $rg $W|awk -v nm=$nm -v rg=$rg '/^mean_lshareA/{printf "%s\t%s\t%s\t%s\t%s\t%s\n",nm,rg,$2,$4,$6,$8}';done; } > "$o/lesions.txt"
