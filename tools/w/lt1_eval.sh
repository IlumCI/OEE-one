#!/usr/bin/env bash
# LT1 measures (docs/WORLD.md, LT1 protocol) for one run (an arm or a control), in the controls' training world:
# waste headroom (gate_all and gate_eat at epochs 1000, 1250, 1500, 1750, 2000 and their mean), futile share and
# conditioning at 2000 (probe), head-to-head at 2000 against each control champion, progress 2000 against 1000.
# Genomes carry their own input normalisation (egg_E.bin.nrm).
# Usage: tools/w/lt1_eval.sh BIN RUN CONTROLRUN... [env: SEEDS=16 THREADS=1 E=2000]
set -u;bin="$1";a="$2";shift 2;n="${SEEDS:-16}";T="${THREADS:-1}";e="${E:-2000}";d=$(dirname "$0");W=$("$d"/wflags.sh "$1")
t(){ "$bin" -tourney "$@" -seeds $n -threads $T $W|awk '/^mean_lshareA/{printf "%s\t%s\t%s\t%s",$2,$4,$6,$8}'; }
echo "== $a (world of $1)"
for ab in gate_all gate_eat;do s=0;k=0;for x in $((e/2)) $((e*5/8)) $((e*3/4)) $((e*7/8)) $e;do g="$a/archive/egg_$x.bin";[ -f "$g" ]||continue
  r=$(t "$g" "$g" -ablate $ab);echo -e "$ab\t$x\t$r";s=$(echo "$s $(echo "$r"|cut -f1)"|awk '{print $1+$2}');k=$((k+1));done
  echo -e "$ab\tmean\t$(echo "$s $k"|awk '{printf "%.4f",$2?$1/$2:0}')";done
g="$a/archive/egg_$e.bin"
for c in "$@";do [ "$c" = "$a" ]&&continue;echo -e "vs\t$c\t$(t "$g" "$c/archive/egg_$e.bin")";done
echo -e "vs_own\t$((e/2))\t$(t "$g" "$a/archive/egg_$((e/2)).bin")"
p=$(mktemp -d);"$bin" -tourney "$g" "$g" -seeds 2 -threads $T -probe $p/x.prb -probe_games 4 $W >/dev/null
python3 "$d"/probe.py -names $(basename $a) $p/x.prb|grep -E '^(futile per agent-tick|P\(eat \| (no food|food underfoot >= 32|food underfoot 2-30)|P\(attack \| (other|kin)|entropy)';rm -rf $p
