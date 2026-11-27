#!/usr/bin/env bash
# LL1 measures (docs/LIFE.md) for one run with lifetime learning, in its training world (wflags.sh, including the
# -life settings): learning used (the champion frozen against an intact copy), waste headroom (gate_all, gate_eat at
# epochs E/2..E in fresh arenas, and at E in the run's own final world), futile share by age (probe), strength against
# control champions (which do not learn: -life_b 0), progress against the run's own champion at E/2.
# Usage: tools/w/ll1_eval.sh BIN RUN CONTROLRUN... [env: SEEDS=16 THREADS=1 E=2000]
set -u;bin="$1";a="$2";shift 2;n="${SEEDS:-16}";T="${THREADS:-1}";e="${E:-2000}";d=$(dirname "$0");W=$("$d"/wflags.sh "$a")
t(){ "$bin" -tourney "$@" -seeds $n -threads $T $W|awk '/^mean_lshareA/{printf "%s\t%s\t%s\t%s",$2,$4,$6,$8}'; }
g="$a/archive/egg_$e.bin";echo "== $a ($W)"
echo -e "freeze\t$e\t$(t "$g" "$g" -ablate freeze)"
for ab in gate_all gate_eat;do s=0;k=0;for x in $((e/2)) $((e*5/8)) $((e*3/4)) $((e*7/8)) $e;do q="$a/archive/egg_$x.bin";[ -f "$q" ]||continue
  r=$(t "$q" "$q" -ablate $ab);echo -e "$ab\t$x\t$r";s=$(echo "$s $(echo "$r"|cut -f1)"|awk '{print $1+$2}');k=$((k+1));done
  echo -e "$ab\tmean\t$(echo "$s $k"|awk '{printf "%.4f",$2?$1/$2:0}')"
  echo -e "$ab\town_world\t$(t "$g" "$g" -ablate $ab -arena_ck $a/ck.bin)";done
for c in "$@";do echo -e "vs\t$c\t$(t "$g" "$c/archive/egg_$e.bin" -life_b 0)";done
echo -e "vs_own\t$((e/2))\t$(t "$g" "$a/archive/egg_$((e/2)).bin")"
p=$(mktemp -d);"$bin" -tourney "$g" "$g" -seeds 2 -threads $T -probe $p/x.prb -probe_games 4 $W >/dev/null
python3 "$d"/probe.py -names $(basename $a) $p/x.prb|grep -E '^(futile|  n age|P\(eat \| (no food|food underfoot >= 32|food underfoot 2-30)|P\(attack \| (other|kin)|entropy|records)';rm -rf $p
