#!/usr/bin/env bash
# Signals + plasticity (docs/PLASTIC.md): resume equivalence, serial vs parallel identity.
# Usage: tests/check_plastic.sh <bin> <work_dir>
set -euo pipefail
bin="$(realpath "$1")";work="$2";rm -rf "$work";mkdir -p "$work"/{A,B,C}
o=(-seed 5 -length 24 -neurons 40 -stats -log log.txt -second-shadow -crng -collectives -division-of-labour -senescence -signals -plastic)
(cd "$work/A"&&"$bin" "${o[@]}" -stopat 6000 2>/dev/null)||[ $? -eq 100 ]
(cd "$work/B"&&"$bin" "${o[@]}" -stopat 3500 -save ck -save-every 2000 2>/dev/null)||[ $? -eq 100 ]
(cd "$work/B"&&"$bin" "${o[@]}" -stopat 6000 -save ck -save-every 2000 -resume ck 2>/dev/null)||[ $? -eq 100 ]
(cd "$work/C"&&"$bin" "${o[@]}" -stopat 6000 -threads 2 2>/dev/null)||[ $? -eq 100 ]
f=0;for x in log.txt;do cmp -s "$work/A/$x" "$work/B/$x"||{ echo "MISMATCH resume $x";f=1;};cmp -s "$work/A/$x" "$work/C/$x"||{ echo "MISMATCH threads $x";f=1;};done
[ $f -eq 0 ]&&echo "plastic: resume and threads equivalent";exit $f
