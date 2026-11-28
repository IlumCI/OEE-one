#!/usr/bin/env bash
# AR5 measures (docs/ARTIFACTS.md) for one run in its training world (wflags.sh): freeze and noteach (learning runs),
# against the E champion of the same seed (B does not learn), LT against L (B does not teach), knowledge (tokens per
# opening, openings, art energy, by speaker age), progress against epoch E/2. Diagnostics (not criteria; added 2026-11-28
# after interim head-to-heads were decided by fights): the same comparisons solo (-solo 1: each side alone in the same
# arenas; log ratio of post-founder biomass, A over B).
# Usage: tools/w/ar5_eval.sh BIN RUN [ERUN] [LRUN] [env: SEEDS=16 THREADS=1 E=2000]
set -u;bin="$1";a="$2";er="${3:-}";lr="${4:-}";n="${SEEDS:-16}";T="${THREADS:-1}";e="${E:-2000}";d=$(dirname "$0");W=$("$d"/wflags.sh "$a")
t(){ "$bin" -tourney "$@" -seeds $n -threads $T $W|awk '/^mean_lshareA/{printf "%s\t%s\t%s\t%s",$2,$4,$6,$8}'; }
ts(){ "$bin" -tourney "$@" -solo 1 -seeds $n -threads $T $W|awk '/^solo_log_ratio/{printf "%s\t%s\t%s\t%s",$2,$4,$6,$8}'; }
g="$a/archive/egg_$e.bin";echo "== $a ($W)"
if echo "$W"|grep -q -- '-life 1';then
  echo -e "freeze\t$e\t$(t "$g" "$g" -ablate freeze)"
  echo "$W"|grep -q -- '-life_teach 1'&&echo -e "noteach\t$e\t$(t "$g" "$g" -ablate noteach)"
  [ -n "$er" ]&&echo -e "vs_E\t$er\t$(t "$g" "$er/archive/egg_$e.bin" -life_b 0)"
  [ -n "$lr" ]&&echo -e "vs_L\t$lr\t$(t "$g" "$lr/archive/egg_$e.bin" -teach_b 0)"
  echo -e "freeze_solo\t$e\t$(ts "$g" "$g" -ablate freeze)"
  echo "$W"|grep -q -- '-life_teach 1'&&echo -e "noteach_solo\t$e\t$(ts "$g" "$g" -ablate noteach)"
  [ -n "$er" ]&&echo -e "vs_E_solo\t$er\t$(ts "$g" "$er/archive/egg_$e.bin" -life_b 0)"
  [ -n "$lr" ]&&echo -e "vs_L_solo\t$lr\t$(ts "$g" "$lr/archive/egg_$e.bin" -teach_b 0)"
fi
echo -e "knowledge\t$e\t$("$bin" -tourney "$g" "$g" -seeds $n -threads $T $W -stats 1|awk '/^stats A/{o="";for(i=3;i<=NF;++i){if($i~/^(symbol|art_tokens\/kt|art_openings\/kt|art_energy\/kt|tokens_per_opening)$/)o=o" "$i" "$(i+1);if($i~/^art_by_age/){for(j=i+1;j<=i+6&&j<=NF;++j)o=o" "$j}}print o}')"
echo -e "vs_own\t$((e/2))\t$(t "$g" "$a/archive/egg_$((e/2)).bin")"
