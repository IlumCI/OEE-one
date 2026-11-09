#!/usr/bin/env bash
# One-line status of a BFF sweep: per seed, last logged epoch and high-order entropy.
out="${1:-runs4/bff}"
for d in "$out"/seed_*; do
  [ -f "$d/log.csv" ] || continue
  l=$(tail -1 "$d/log.csv"); e=${l%%,*}; he=$(echo "$l" | cut -d, -f4)
  t=$(awk -F, 'NR > 1 && $4 >= 1 {print $1; exit}' "$d/log.csv")
  printf '%s:%s/%s%s%s ' "${d##*seed_}" "$e" "$he" "${t:+ T@$t}" "$([ -f "$d/done" ] && echo ' done')"
done; echo
