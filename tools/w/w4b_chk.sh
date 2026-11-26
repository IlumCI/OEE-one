#!/usr/bin/env bash
# Adapter for w3_when.sh (CHK=tools/w/w4b_chk.sh): W4b measures (docs/WORLD.md §6, W4b protocol) for runs8/w4f_sN at
# epoch e: tools/w/w3x_eval.sh, and F's champion against W4a E's (same seed) head-to-head in F's world and
# in E's world: h2h_F_vs_E world lshareA se t wins (post-founder share of F).
set -u;bin="$1";r="$2";e="$3";n="${4:-16}";T="${5:-1}";d=$(dirname "$0")
"$d"/w3x_eval.sh "$bin" "$r" "$e" "$n" "$T"
case "$r" in *w4f_s*) dr=$(echo "$r"|sed 's/w4f_s/w4e_s/')
  while [ ! -f "$dr/archive/egg_$e.bin" ];do sleep 60;done
  for wr in "$r" "$dr";do W=$("$d"/wflags.sh "$wr")
    "$bin" -tourney "$r/archive/egg_$e.bin" "$dr/archive/egg_$e.bin" -seeds $n -threads $T $W|
    awk -v w="$(basename "$wr")" '/^mean_lshareA/{printf "h2h_F_vs_E\tworld %s\t%s\t%s\t%s\t%s\n",w,$2,$4,$6,$8}'
  done;;esac
