#!/usr/bin/env bash
# Adapter for w3_when.sh (CHK=tools/w/w4c_chk.sh): W4c measures (docs/WORLD.md §6, W4c protocol) for runs8/w4g_sN at
# its final epoch e: tools/w/w3x_eval.sh, and G's champion against W4a E's final champion (epoch 1000, same seed,
# equal ticks) head-to-head in G's world and in E's world: h2h_G_vs_E world lshareA se t wins (post-founder share of G).
set -u;bin="$1";r="$2";e="$3";n="${4:-16}";T="${5:-1}";d=$(dirname "$0")
"$d"/w3x_eval.sh "$bin" "$r" "$e" "$n" "$T"
dr=$(echo "$r"|sed 's/w4g_s/w4e_s/')
for wr in "$r" "$dr";do W=$("$d"/wflags.sh "$wr")
  "$bin" -tourney "$r/archive/egg_$e.bin" "$dr/archive/egg_1000.bin" -seeds $n -threads $T $W|
  awk -v w="$(basename "$wr")" '/^mean_lshareA/{printf "h2h_G_vs_E\tworld %s\t%s\t%s\t%s\t%s\n",w,$2,$4,$6,$8}'
done
