#!/usr/bin/env bash
# W3.x pilot measures at epoch e (docs/WORLD.md §6, W3.x pilot protocol), in the run's training world (wflags.sh):
# kill-test lines (w3_check.sh), generality at e against e-500 (generality.sh: solo, head-to-head, swap), era progress
# (e against e-250, post-founder share) and the census row of the log at e.
# Usage: tools/w/w3x_eval.sh BIN RUNDIR EPOCH [seeds=16] [threads=2]
set -u
bin="$1";run="$2";e="$3";n="${4:-16}";T="${5:-2}";d=$(dirname "$0");W=$("$d"/wflags.sh "$run")
"$d"/w3_check.sh "$bin" "$run" "$e" "$n" "$T"
echo "== generality (e vs e-500)";ONLY=$e "$d"/generality.sh "$bin" "$run" 500 64 "$T" 24 $W
echo "== era (post-founder share, e vs e-250)";"$d"/w1_eval.sh "$bin" "$run" 250 "$n" "$T" $W|awk -v e=$e 'NR>1&&$1<=e{printf "%s:%.2f ",$1,$8}END{print ""}'
echo "== census (epoch machines walls tapes classes)";awk -v e=$e '!/^#/&&$1==e{print $1,$(NF-3),$(NF-2),$(NF-1),$NF}' "$run/log.txt"
m="${MCAL:-build/rel/mcal}";[ -x "$m" ]&&[ -f "$run/ck.bin" ]&&{ echo "== machines by owner in the latest checkpoint (mcal -ck)";"$m" -ck "$run/ck.bin"; }
