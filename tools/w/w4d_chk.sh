#!/usr/bin/env bash
# Adapter for w3_when.sh (CHK=tools/w/w4d_chk.sh): W4d measures (docs/WORLD.md, W4d protocol) for runs8/w4h_sN at
# epoch e: tools/w/w3x_eval.sh; the no-feed ablation (share of the copy that cannot feed) in fresh arenas and in the
# run's own final world; H's champion against W4a E's final champion (same seed) head-to-head in both worlds; and
# the land census of the final world. Usage: tools/w/w4d_chk.sh BIN RUNDIR EPOCH [seeds] [threads]
set -u;bin="$1";r="$2";e="$3";n="${4:-16}";T="${5:-1}";d=$(dirname "$0")
"$d"/w3x_eval.sh "$bin" "$r" "$e" "$n" "$T"
W=$("$d"/wflags.sh "$r");g="$r/archive/egg_$e.bin"
for a in fresh own;do x="";[ $a = own ]&&x="-arena_ck $r/ck.bin"
  "$bin" -tourney "$g" "$g" -seeds $n -threads $T -ablate nofeed $x $W|awk -v a=$a '/^mean_lshareA/{printf "nofeed\tworld %s\t%s\t%s\t%s\t%s\n",a,$2,$4,$6,$8}';done
dr=$(echo "$r"|sed 's/w4h_s/w4e_s/')
for wr in "$r" "$dr";do W2=$("$d"/wflags.sh "$wr")
  "$bin" -tourney "$g" "$dr/archive/egg_1000.bin" -seeds $n -threads $T $W2|
  awk -v w="$(basename "$wr")" '/^mean_lshareA/{printf "h2h_H_vs_E\tworld %s\t%s\t%s\t%s\t%s\n",w,$2,$4,$6,$8}'
done
c="${IRRC:-build/rel/irrcensus}";[ -x "$c" ]&&{ echo "== land and irrigation in the final world";"$c" "$r/ck.bin"; }
python3 "$d"/booms.py -e0 200 -e1 1000 "$r" "$dr"
