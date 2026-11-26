#!/usr/bin/env bash
# Adapter for w3_when.sh (CHK=tools/w/w4a_chk.sh): W4a measures (docs/WORLD.md §6, W4a protocol) for runs8/w4{d,e}_sN at
# epoch e: tools/w/w3x_eval.sh, and for E runs also E's champion against D's (same seed) head-to-head in E's world and
# in D's world: h2h_E_vs_D world lshareA se t wins (post-founder share of E).
set -u;bin="$1";r="$2";e="$3";n="${4:-16}";T="${5:-1}";d=$(dirname "$0")
"$d"/w3x_eval.sh "$bin" "$r" "$e" "$n" "$T"
case "$r" in *w4e_s*) dr=$(echo "$r"|sed 's/w4e_s/w4d_s/')
  while [ ! -f "$dr/archive/egg_$e.bin" ];do sleep 60;done
  for wr in "$r" "$dr";do W=$("$d"/wflags.sh "$wr")
    "$bin" -tourney "$r/archive/egg_$e.bin" "$dr/archive/egg_$e.bin" -seeds $n -threads $T $W|
    awk -v w="$(basename "$wr")" '/^mean_lshareA/{printf "h2h_E_vs_D\tworld %s\t%s\t%s\t%s\t%s\n",w,$2,$4,$6,$8}'
  done;;esac
