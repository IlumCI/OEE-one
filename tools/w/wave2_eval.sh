#!/usr/bin/env bash
# Wave-2 measures at epoch e (docs/WORLD.md §6, wave 2 protocol): an arm's champion against C's champion on the same
# seed. Head-to-head (post-founder share of the arm) in the arm's training world and, if the worlds differ (kin_sig,
# lamarck), also in C's world; solo generality of the arm against C in generated arenas (-gen 1 -arena_progs 24, 64
# seeds, the arm's world); then the arm's own kill-test lines and era progress (w3_check.sh, w1_eval.sh).
# Usage: tools/w/wave2_eval.sh BIN ARM_RUN C_RUN [epoch=2000] [seeds=16] [threads=2]
set -u
bin="$1";a="$2";c="$3";e="${4:-2000}";n="${5:-16}";T="${6:-2}";d=$(dirname "$0");Wa=$("$d"/wflags.sh "$a");Wc=$("$d"/wflags.sh "$c")
A="$a/archive/egg_$e.bin";C="$c/archive/egg_$e.bin";echo "# $a vs $c e $e"
h2h(){ "$bin" -tourney "$A" "$C" -seeds $n -threads $T $2|awk -v w=$1 '/^mean_lshareA/{printf "h2h\t%s\t%s\t%s\t%s\t%s\n",w,$2,$4,$6,$8}'; }
h2h arm "$Wa";[ "$Wa" != "$Wc" ]&&h2h C "$Wc"
"$bin" -tourney "$A" "$C" -solo 1 -gen 1 -arena_progs 24 -ticks 2000 -seeds 64 -threads $T $Wa|awk '/^solo_log_ratio/{printf "solo\t%s\t%s\t%s\t%s\n",$2,$4,$6,$8}'
"$d"/w3_check.sh "$bin" "$a" "$e" "$n" "$T"
echo "== era (post-founder share, e vs e-250)";"$d"/w1_eval.sh "$bin" "$a" 250 "$n" "$T" $Wa|awk -v e=$e 'NR>1&&$1<=e{printf "%s:%.2f ",$1,$8}END{print ""}'
