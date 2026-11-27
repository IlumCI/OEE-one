#!/usr/bin/env bash
# Adapter for w3_when.sh (CHK=tools/w/w4e_chk.sh): W4e-W4g measures (with -home_cue 1 also the nocue ablation) (docs/WORLD.md, W4e protocol) for runs8/w4i_sN at
# epoch e: tools/w/w3x_eval.sh (its stats lines include 'home', the share of agent-ticks on own land, for the
# champion and for random agents); the no-feed ablation (share of the copy that cannot feed) in fresh homestead
# arenas and in the run's own final world; the land census of the final world; boom statistics beside W4d arm H.
# Usage: tools/w/w4e_chk.sh BIN RUNDIR EPOCH [seeds] [threads]
set -u;bin="$1";r="$2";e="$3";n="${4:-16}";T="${5:-1}";d=$(dirname "$0")
"$d"/w3x_eval.sh "$bin" "$r" "$e" "$n" "$T"
W=$("$d"/wflags.sh "$r");g="$r/archive/egg_$e.bin"
for ab in nofeed nocue;do [ $ab = nocue ]&&! echo "$W"|grep -q -- '-home_cue 1'&&continue
for a in fresh own;do x="";[ $a = own ]&&x="-arena_ck $r/ck.bin"
  "$bin" -tourney "$g" "$g" -seeds $n -threads $T -ablate $ab $x $W|awk -v a=$a -v ab=$ab '/^mean_lshareA/{printf "%s\tworld %s\t%s\t%s\t%s\t%s\n",ab,a,$2,$4,$6,$8}';done;done
c="${IRRC:-build/rel/irrcensus}";[ -x "$c" ]&&{ echo "== land and irrigation in the final world";"$c" "$r/ck.bin"; }
python3 "$d"/booms.py -e0 200 -e1 1000 "$r" "$(echo "$r"|sed 's/w4i_s/w4h_s/;s/w4j_s/w4i_s/;s/w4k_s/w4j_s/')"
