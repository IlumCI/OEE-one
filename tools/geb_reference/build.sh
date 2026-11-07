#!/usr/bin/env bash
# Build the vendored Geb v08b3 reference binaries out of tree.
# Usage: tools/geb_reference/build.sh [build_dir]   (default: build/geb_reference)
set -euo pipefail

root="$(cd "$(dirname "$0")/../.." && pwd)"
src="$root/third_party/geb_v08b3"
out="${1:-$root/build/geb_reference}"
mkdir -p "$out"

# Original flags from third_party/geb_v08b3/makefile. -ffloat-store is required for
# run-to-run reproducibility on x87; kept for bit-compatibility with published runs.
cxx="${CXX:-g++}"
flags=(-O3 -ffloat-store -w -DgebLinux)

for f in rnd_core ann_core geb_core geb_file win_xwin ann_xwin geb_xwin; do
  "$cxx" "${flags[@]}" -c "$src/$f.cpp" -o "$out/$f.o"
done
"$cxx" "${flags[@]}" -Dreset_shadow_to_real_for_each_pass      -c "$src/geb_rs.cpp" -o "$out/geb_rs_resets.o"
"$cxx" "${flags[@]}" -Ddont_reset_shadow_to_real_for_each_pass -c "$src/geb_rs.cpp" -o "$out/geb_rs_no_resets.o"

common=(rnd_core.o ann_core.o geb_core.o geb_file.o win_xwin.o ann_xwin.o geb_xwin.o)
(cd "$out" && "$cxx" -o geb_resets    "${common[@]}" geb_rs_resets.o    -lX11 -lm)
(cd "$out" && "$cxx" -o geb_no_resets "${common[@]}" geb_rs_no_resets.o -lX11 -lm)

echo "$out/geb_resets"
echo "$out/geb_no_resets"
