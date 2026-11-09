#!/usr/bin/env bash
# Build the showcase site and media from a run checkpoint.
# Usage: tools/showcase/build.sh CHECKPOINT BUILD_DIR [STEPS]
# Requires: build of target `replay`, node + playwright (media), ffmpeg (media).
set -euo pipefail
ck="$1"; bin="$2"; steps="${3:-900}"
here="$(cd "$(dirname "$0")" && pwd)"; root="$here/../.."
tmp="$(mktemp -d)"
"$bin/replay" "$ck" "$steps" 1 > "$tmp/replay.json"
python3 -I "$here/encode.py" "$tmp/replay.json" "$here/template.html" "$root/site/index.html"
[ -n "${NO_MEDIA:-}" ] && exit 0
cp "$root/site/index.html" "$tmp/wrap.html"; cp "$here/record.js" "$tmp/"
mkdir -p "$tmp/g" "$tmp/h" "$tmp/v" "$root/site/media"
(cd "$tmp" && node record.js 960 540 20 12 g && node record.js 1920 1080 30 20 h && node record.js 1080 1920 30 20 v)
ffmpeg -loglevel error -y -framerate 20 -i "$tmp/g/f%04d.png" \
  -vf "split[a][b];[a]palettegen=max_colors=32:stats_mode=full[p];[b][p]paletteuse=dither=none" -loop 0 "$root/site/media/oee-one.gif"
for v in h:1080p v:vertical; do
  ffmpeg -loglevel error -y -framerate 30 -i "$tmp/${v%%:*}/f%04d.png" -c:v libx264 -pix_fmt yuv420p -crf 26 \
    -tune animation -preset slow -movflags +faststart "$root/site/media/oee-one-${v##*:}.mp4"
done
echo "site/index.html and site/media/ written"
