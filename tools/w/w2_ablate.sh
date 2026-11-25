#!/usr/bin/env bash
# W2a ablations (docs/WORLD.md §6, W2a protocol): a champion against its intact copy, mirrored seeds.
# Usage: tools/w/w2_ablate.sh BIN RUNDIR [epoch=last] [seeds=16] [threads=4] [training world flags...]
# Prints, for amnesia and (with signals) deaf, in the training arenas and in arenas with seasons off (control):
#   ablation arena ishareA se t lshareA lse lt   (shares of the ablated copy; < 0.5 means the mechanism pays)
set -u
bin="$1";run="$2";e="${3:-}";n="${4:-16}";T="${5:-4}";shift 5||shift $#;W="$*"
[ -n "$e" ]||e=$(ls "$run"/archive/egg_*.bin|sed 's/.*egg_\([0-9]*\).bin/\1/'|sort -n|tail -1)
g="$run/archive/egg_$e.bin";echo "# champion $g; world: $W"
sig=0;case " $W " in *" -signals 1 "*)sig=1;;esac
static=$(echo " $W "|sed 's/ -season [0-9]*/ -season 0/')
for ab in amnesia deaf;do [ $ab = deaf ]&&[ $sig = 0 ]&&continue
  for arena in seasons static;do w="$W";[ $arena = static ]&&w="$static"
    "$bin" -tourney "$g" "$g" -seeds $n -threads $T -ablate $ab $w|
    awk -v a=$ab -v r=$arena '/^mean_ishareA/{i=$2"\t"$4"\t"$6}/^mean_lshareA/{printf "%s\t%s\t%s\t%s\t%s\t%s\n",a,r,i,$2,$4,$6}'
  done;done
