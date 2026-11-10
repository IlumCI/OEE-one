#!/usr/bin/env bash
# Resumable BFF seed sweep (paper default configuration).
# Usage: [EXTRA="-grid 240 135"] tools/bff/sweep.sh OUT_DIR BIN SEED_FIRST SEED_LAST [THREADS]
# Each seed: up to 16384 epochs, metrics every 64 epochs, stops 2048 epochs after
# high-order entropy first reaches 1 (the paper's transition criterion). Checkpoints
# every 512 epochs; on restart the log is truncated to the checkpoint and resumed.
set -uo pipefail
out="$1"; bin="$2"; first="$3"; last="$4"; threads="${5:-1}"
mkdir -p "$out"
for s in $(seq "$first" "$last"); do
  d="$out/seed_$s"; mkdir -p "$d"
  [ -f "$d/done" ] && continue
  args=(-seed "$s" ${EXTRA:-} -epochs ${EPOCHS:-16384} -interval 64 -threads "$threads" -log "$d/log.csv"
        -save "$d/ck" -save-every 512 -stop-entropy 1 -stop-after 2048)
  if [ -f "$d/ck" ]; then
    ep=$(od -An -t u8 -j 32 -N 8 "$d/ck" | tr -d ' ')
    awk -F, -v e="$ep" 'NR == 1 || $1 <= e' "$d/log.csv" > "$d/log.tmp" && mv "$d/log.tmp" "$d/log.csv"
    args+=(-resume "$d/ck")
  fi
  "$bin" "${args[@]}"; code=$?
  if [ "$code" = 0 ] || [ "$code" = 10 ]; then echo "$code" > "$d/done"; else echo "seed $s exited $code" >&2; exit 1; fi
done
