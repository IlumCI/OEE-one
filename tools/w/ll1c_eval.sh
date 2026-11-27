#!/usr/bin/env bash
# LL1c measures (docs/LIFE.md) for one run in its training world (wflags.sh): learning used (champion frozen against an
# intact copy; norew too for an evolved-reward run), learning helps evolution (champion against the E champion of the
# same seed, which does not learn: -life_b 0), the learning curve (toxic share of eats by age within an era, intact and
# frozen), progress against the run's own champion at E/2. For an E run: the curve and progress only.
# Usage: tools/w/ll1c_eval.sh BIN RUN [ERUN] [env: SEEDS=16 THREADS=1 E=2000]
set -u;bin="$1";a="$2";er="${3:-}";n="${SEEDS:-16}";T="${THREADS:-1}";e="${E:-2000}";d=$(dirname "$0");W=$("$d"/wflags.sh "$a")
t(){ "$bin" -tourney "$@" -seeds $n -threads $T $W|awk '/^mean_lshareA/{printf "%s\t%s\t%s\t%s",$2,$4,$6,$8}'; }
cv(){ "$bin" -tourney "$@" -seeds $n -threads $T $W -stats 1|awk '/^stats A/{for(i=1;i<=NF;++i){if($i=="toxic_share")s=$(i+1);if($i=="toxic_by_age"){c="";for(j=i+1;j<=NF;++j)c=c" "$j}};printf "%s\t%s",s,c}'; }
g="$a/archive/egg_$e.bin";echo "== $a ($W)"
if echo "$W"|grep -q -- '-life 1';then
  echo -e "freeze\t$e\t$(t "$g" "$g" -ablate freeze)"
  echo "$W"|grep -q -- '-life_rew 1'&&echo -e "norew\t$e\t$(t "$g" "$g" -ablate norew)"
  [ -n "$er" ]&&echo -e "vs_E\t$er\t$(t "$g" "$er/archive/egg_$e.bin" -life_b 0)"
  echo -e "curve_frozen\t$e\t$(cv "$g" "$g" -ablate freeze)"
fi
echo -e "curve\t$e\t$(cv "$g" "$g")"
echo -e "vs_own\t$((e/2))\t$(t "$g" "$a/archive/egg_$((e/2)).bin")"
