#!/bin/bash
# Restore an archived run directory from the 'run-data' branch (fetches only that run's parts).
# Usage: tools/storage/restore_run.sh RUNDIR   (e.g. runs6/gebgrow_s1)
set -eu
repo=$(git rev-parse --show-toplevel);p=$(realpath -m "$1");p=${p#"$repo"/}
url=$(git -C "$repo" remote get-url origin);tmp=$(mktemp -d)
git init -q "$tmp/g";cd "$tmp/g";git remote add origin "$url"
# run-data-hist anchors the history before 2026-11-20, when a failed archiver fetch replaced run-data
git fetch -q --filter=blob:none origin run-data:refs/remotes/o/rd run-data-hist:refs/remotes/o/rh 2>/dev/null||git fetch -q --filter=blob:none origin run-data:refs/remotes/o/rd
# the newest commit that adds this run (older archive commits did not keep earlier runs in the tip tree)
c=$(git log -1 --diff-filter=AM --format=%H $(git for-each-ref --format='%(refname)' refs/remotes/o) -- "data/$p/SHA256SUMS");[ -n "$c" ]||{ echo "not on run-data: $p";exit 1;}
for f in $(git ls-tree --name-only "$c" "data/$p/"|sort);do git show "$c:$f">"$tmp/${f##*/}";done
cd "$tmp";sha256sum -c --quiet SHA256SUMS
mkdir -p "$(dirname "$repo/$p")";rm -f "$repo/$p/ARCHIVED"
cat "$tmp"/run.tar.xz.part*|xz -dc|tar -C "$(dirname "$repo/$p")" -xf -
rm -rf "$tmp";echo "restored $p"
