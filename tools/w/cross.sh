#!/usr/bin/env bash
# Champions of run A against champions of run B at the same epochs (mirrored seeds), e.g. W1b against W1 (WORLD.md).
# Usage: tools/w/cross.sh BIN RUN_A RUN_B "500 1000 1500 2000" [seeds=16] [threads=4] [world flags, default W1]
# Output (TSV): epoch ishareA se t lshareA lse lt lwins   (shares of A's champion)
set -u
bin="$1";a="$2";b="$3";E="$4";n="${5:-16}";T="${6:-4}";shift 6||shift $#;W="${*:--S 64 -regrow 2 -nact 7 -ticks 2000}"
printf "epoch\tishareA\tse\tt\tlshareA\tlse\tlt\tlwins\n"
for e in $E;do [ -f "$a/archive/egg_$e.bin" ]&&[ -f "$b/archive/egg_$e.bin" ]||continue
  "$bin" -tourney "$a/archive/egg_$e.bin" "$b/archive/egg_$e.bin" -seeds $n -threads $T $W|
  awk -v e=$e '/^mean_ishareA/{i=$2"\t"$4"\t"$6}/^mean_lshareA/{printf "%d\t%s\t%s\t%s\t%s\t%s\n",e,i,$2,$4,$6,$8}'
done
