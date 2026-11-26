#!/usr/bin/env bash
# W3 generality (docs/WORLD.md §6): champions at epoch e against e-D in arenas the run never saw. Each arena draws its
# own world settings (-gen 1) and is seeded with random machine programs, short ones likelier (-arena_progs).
# Two scores: solo (each genome alone in the same arenas: mean log ratio of post-founder biomass, survival counts;
# an absolute measure in the spirit of Legg & Hutter, arXiv:0712.3329) and head-to-head (post-founder share).
# Third score, swap test: at tick 1000 the arena's programs are replaced by new random ones; recovery of each genome
# (biomass in the 500 ticks after / the 500 before), as the mean log ratio of the later champion's to the earlier's.
# Usage: tools/w/generality.sh BIN RUNDIR [D=500] [seeds=64] [threads=4] [progs=24] [training world flags...]
# ONLY=e in the environment scores epoch e alone.
# Output (TSV): e e_prev solo_log_ratio t median surviveA surviveB lshareA lt swap_recovery swap_t
set -u
bin="$1";run="$2";D="${3:-500}";n="${4:-64}";T="${5:-4}";P="${6:-24}";shift 6||shift $#
W="${*:--S 64 -regrow 16 -nact 10 -matter 1 -season 300 -view 1 -signals 1}"
printf "e\te_prev\tsolo_log_ratio\tt\tmedian\tsurviveA\tsurviveB\tlshareA\tlt\tswap_recovery\tswap_t\n"
for e in $(ls "$run"/archive/egg_*.bin|sed 's/.*egg_\([0-9]*\).bin/\1/'|sort -n);do
  b=$((e-D));[ $((e%D)) = 0 ]&&[ $b -ge 0 ]&&[ -f "$run/archive/egg_$b.bin" ]||continue
  [ -n "${ONLY:-}" ]&&[ "$e" != "$ONLY" ]&&continue
  A="$run/archive/egg_$e.bin";B="$run/archive/egg_$b.bin";G="-gen 1 -arena_progs $P -ticks 2000 -seeds $n -threads $T $W"
  so=$("$bin" -tourney "$A" "$B" -solo 1 $G|awk '/^solo_log_ratio/{print $2"\t"$6"\t"$10"\t"$12"\t"$14}')
  hh=$("$bin" -tourney "$A" "$B" $G|awk '/^mean_lshareA/{print $2"\t"$6}')
  sw=$("$bin" -tourney "$A" "$B" -solo 1 -swap_at 1000 -swap_win 500 $G|awk '/^swap_recovery/{print $2"\t"$6}')
  printf "%d\t%d\t%s\t%s\t%s\n" $e $b "$so" "$hh" "$sw"
done
