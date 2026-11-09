#!/usr/bin/env bash
# Build the showcase site and media from a run checkpoint.
# Usage: tools/showcase/build.sh CHECKPOINT BUILD_DIR [STEPS]
# Env BFF_SEED (default 0), BFF_FROM, BFF_TO (default 2304, 3456): the BFF run replayed for
# the origin scene (it must transition inside that window; seed 0 does at epoch 2433).
# Requires: build of target `replay`, node + playwright (media), ffmpeg (media).
set -euo pipefail
ck="$1"; bin="$2"; steps="${3:-900}"
here="$(cd "$(dirname "$0")" && pwd)"; root="$here/../.."
tmp="$(mktemp -d)"
"$bin/replay" "$ck" "$steps" 1 > "$tmp/replay.json"
bseed="${BFF_SEED:-0}"; bfrom="${BFF_FROM:-2304}"; bto="${BFF_TO:-3456}"
mkdir -p "$tmp/bff"
"$bin/oee_bff" -seed "$bseed" -epochs "$bto" -interval 64 -dump-every 8 -dump-from "$bfrom" -dump-dir "$tmp/bff" \
  -log "$tmp/bff/log.csv" -threads "${THREADS:-1}"
python3 -I "$here/encode_bff.py" "$tmp/bff" "$bfrom" "$bto" "$tmp/bff.json" --log "$tmp/bff/log.csv"
python3 -I "$here/encode.py" "$tmp/replay.json" "$here/template.html" "$root/site/index.html" "$tmp/bff.json"
[ -n "${NO_MEDIA:-}" ] && exit 0
cp "$root/site/index.html" "$tmp/wrap.html"; cp "$here/record.js" "$tmp/"
mkdir -p "$tmp/g" "$tmp/h" "$tmp/v" "$root/site/media"
mkdir -p "$tmp/og" "$tmp/oh" "$tmp/ov"
(cd "$tmp" && HASH='#agents' node record.js 960 540 20 12 g && HASH='#agents' node record.js 1920 1080 30 20 h \
  && HASH='#agents' node record.js 1080 1920 30 20 v && HASH='#origin' node record.js 960 540 12 20 og \
  && HASH='#origin' node record.js 1920 1080 30 20 oh && HASH='#origin' node record.js 1080 1920 30 20 ov)
ffmpeg -loglevel error -y -framerate 20 -i "$tmp/g/f%04d.png" \
  -vf "split[a][b];[a]palettegen=max_colors=32:stats_mode=full[p];[b][p]paletteuse=dither=none" -loop 0 "$root/site/media/oee-one.gif"
ffmpeg -loglevel error -y -framerate 12 -i "$tmp/og/f%04d.png" \
  -vf "split[a][b];[a]palettegen=max_colors=32:stats_mode=full[p];[b][p]paletteuse=dither=none" -loop 0 "$root/site/media/oee-one-origin.gif"
for v in h:1080p v:vertical oh:origin-1080p ov:origin-vertical; do
  ffmpeg -loglevel error -y -framerate 30 -i "$tmp/${v%%:*}/f%04d.png" -c:v libx264 -pix_fmt yuv420p -crf 26 \
    -tune animation -preset slow -movflags +faststart "$root/site/media/oee-one-${v##*:}.mp4"
done
echo "site/index.html and site/media/ written"
